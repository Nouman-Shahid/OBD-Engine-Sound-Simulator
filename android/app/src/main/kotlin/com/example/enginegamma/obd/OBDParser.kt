package com.example.enginegamma.obd

/**
 * ELM327 OBD-II response parser.
 *
 * Handles PID decoding for the two parameters we care about:
 *   - 010C → Engine RPM
 *   - 0111 → Throttle position
 *
 * ELM327 response format:
 *   Each response line is a hex string like "41 0C A0 B0\r\n"
 *   where:
 *     41 = mode 01 response (0x41 = 0x40 + mode)
 *     0C = PID number
 *     A0 B0 = data bytes
 *
 * RPM formula:   RPM = (256×A + B) / 4
 * Throttle formula: throttle% = A × 100/255
 */
object OBDParser {

    // ── AT command set ────────────────────────────────────────────────────────

    /** Initialisation sequence to send after connecting. */
    val INIT_COMMANDS = listOf(
        "ATZ\r",     // Reset
        "ATE0\r",    // Echo off
        "ATL0\r",    // Linefeeds off
        "ATS0\r",    // Spaces off
        "ATH0\r",    // Headers off
        "ATSP0\r",   // Auto protocol
        "0100\r"     // Supported PIDs check (also wakes ECU)
    )

    const val CMD_RPM      = "010C\r"
    const val CMD_THROTTLE = "0111\r"

    // Prompt character ELM327 sends when ready for next command
    const val PROMPT       = ">"

    // ── Response parsers ──────────────────────────────────────────────────────

    /**
     * Parse RPM from ELM327 response.
     * Returns null on parse failure or NO DATA.
     */
    fun parseRpm(response: String): Double? {
        val cleaned = response.trim().uppercase()
        if (cleaned.contains("NO DATA") || cleaned.contains("ERROR") ||
            cleaned.contains("UNABLE") || cleaned.isEmpty()) return null

        // With headers off: "410CA0B0" or "41 0C A0 B0"
        val hex = cleaned.filter { it.isLetterOrDigit() }
        if (!hex.startsWith("410C") || hex.length < 8) return null

        return try {
            val a = hex.substring(4, 6).toInt(16)
            val b = hex.substring(6, 8).toInt(16)
            ((256.0 * a + b) / 4.0)
        } catch (e: NumberFormatException) {
            null
        }
    }

    /**
     * Parse throttle position from ELM327 response.
     * Returns null on failure.
     */
    fun parseThrottle(response: String): Double? {
        val cleaned = response.trim().uppercase()
        if (cleaned.contains("NO DATA") || cleaned.contains("ERROR")) return null

        val hex = cleaned.filter { it.isLetterOrDigit() }
        if (!hex.startsWith("4111") || hex.length < 6) return null

        return try {
            val a = hex.substring(4, 6).toInt(16)
            a.toDouble() / 255.0   // normalised [0, 1]
        } catch (e: NumberFormatException) {
            null
        }
    }

    /**
     * Check if the ELM327 device is done processing (sent the prompt).
     */
    fun isPromptReady(buffer: String): Boolean =
        buffer.contains(PROMPT)
}
