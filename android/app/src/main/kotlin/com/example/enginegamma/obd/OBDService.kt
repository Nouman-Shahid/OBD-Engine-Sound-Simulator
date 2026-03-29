package com.example.enginegamma.obd

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.Context
import android.util.Log
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.util.UUID
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Bluetooth Classic OBD-II service using the ELM327 adapter.
 *
 * Connection flow:
 *  1. [scanPairedDevices] — returns already-paired BT devices as candidates.
 *  2. [connect(address)] — opens an RFCOMM socket to the chosen device.
 *  3. Initialisation sequence (AT commands) configures the adapter.
 *  4. Polling loop reads RPM + throttle every ~100 ms.
 *  5. [disconnect] — closes socket and stops polling.
 *
 * Threading:
 *  - connect() returns immediately; all IO runs on a background thread.
 *  - Callbacks (onConnected, onData, onDisconnected) are fired on that IO thread.
 *    The caller ([EngineChannelHandler]) posts them back to the main thread.
 *
 * Permissions required (declared in AndroidManifest):
 *   Android ≤ 11: BLUETOOTH, BLUETOOTH_ADMIN, ACCESS_FINE_LOCATION
 *   Android 12+:  BLUETOOTH_CONNECT, BLUETOOTH_SCAN
 */
class OBDService(private val context: Context) {

    companion object {
        private const val TAG = "OBDService"
        /** Standard SPP (Serial Port Profile) UUID for ELM327. */
        private val SPP_UUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
        private const val POLL_INTERVAL_MS = 100L
        private const val INIT_TIMEOUT_MS  = 3000L
    }

    private var socket:   BluetoothSocket? = null
    private var inputStream:  InputStream?  = null
    private var outputStream: OutputStream? = null
    private val running = AtomicBoolean(false)
    private var ioThread: Thread? = null

    // ── Public API ────────────────────────────────────────────────────────────

    /**
     * Returns a list of already-paired Bluetooth devices as
     * [Pair(name, address)] for display in the UI device picker.
     */
    @Suppress("MissingPermission")
    fun scanPairedDevices(): List<Map<String, String>> {
        val adapter = BluetoothAdapter.getDefaultAdapter() ?: return emptyList()
        return adapter.bondedDevices.map { device ->
            mapOf("name" to (device.name ?: "Unknown"), "address" to device.address)
        }
    }

    /**
     * Asynchronously connect to an ELM327 device at [address].
     * Fires callbacks on the IO thread (marshal to main thread in caller).
     */
    @Suppress("MissingPermission")
    fun connect(
        address: String,
        onConnected:    () -> Unit,
        onData:         (rpm: Double, throttle: Double) -> Unit,
        onDisconnected: () -> Unit
    ) {
        disconnect() // Close any existing connection

        ioThread = Thread {
            try {
                val adapter = BluetoothAdapter.getDefaultAdapter()
                    ?: throw IOException("Bluetooth not available")
                val device: BluetoothDevice = adapter.getRemoteDevice(address)

                // Cancel discovery to avoid interference
                adapter.cancelDiscovery()

                val s: BluetoothSocket = device.createRfcommSocketToServiceRecord(SPP_UUID)
                s.connect()
                socket       = s
                inputStream  = s.inputStream
                outputStream = s.outputStream
                running.set(true)

                Log.i(TAG, "Connected to $address")
                onConnected()

                // Send initialisation commands
                if (!initialise()) {
                    throw IOException("ELM327 initialisation failed")
                }

                // Polling loop
                pollLoop(onData)

            } catch (e: IOException) {
                Log.e(TAG, "OBD connection error: ${e.message}")
            } finally {
                running.set(false)
                closeStreams()
                onDisconnected()
                Log.i(TAG, "Disconnected from $address")
            }
        }.also { it.isDaemon = true; it.start() }
    }

    fun disconnect() {
        running.set(false)
        ioThread?.interrupt()
        closeStreams()
    }

    fun isConnected(): Boolean = running.get() && socket?.isConnected == true

    // ── Private IO helpers ────────────────────────────────────────────────────

    private fun initialise(): Boolean {
        val out = outputStream ?: return false
        for (cmd in OBDParser.INIT_COMMANDS) {
            try {
                out.write(cmd.toByteArray(Charsets.ISO_8859_1))
                out.flush()
                // Wait for prompt with timeout
                val response = readUntilPrompt(INIT_TIMEOUT_MS)
                Log.d(TAG, "INIT [$cmd] → $response")
                Thread.sleep(100)
            } catch (e: IOException) {
                Log.w(TAG, "Init command failed: $cmd")
                return false
            }
        }
        return true
    }

    private fun pollLoop(onData: (rpm: Double, throttle: Double) -> Unit) {
        val out = outputStream ?: return

        while (running.get() && !Thread.currentThread().isInterrupted) {
            try {
                // Request RPM
                out.write(OBDParser.CMD_RPM.toByteArray(Charsets.ISO_8859_1))
                out.flush()
                val rpmResponse = readUntilPrompt(500)
                val rpm = OBDParser.parseRpm(rpmResponse)

                // Request throttle
                out.write(OBDParser.CMD_THROTTLE.toByteArray(Charsets.ISO_8859_1))
                out.flush()
                val tpsResponse = readUntilPrompt(500)
                val throttle = OBDParser.parseThrottle(tpsResponse)

                if (rpm != null) {
                    onData(rpm, throttle ?: 0.0)
                }

                Thread.sleep(POLL_INTERVAL_MS)

            } catch (e: InterruptedException) {
                Thread.currentThread().interrupt()
                break
            } catch (e: IOException) {
                Log.w(TAG, "Poll IO error: ${e.message}")
                break
            }
        }
    }

    /**
     * Read bytes from the input stream until the ELM327 prompt '>' is received
     * or the [timeoutMs] expires.
     */
    private fun readUntilPrompt(timeoutMs: Long): String {
        val input  = inputStream ?: return ""
        val sb     = StringBuilder()
        val buf    = ByteArray(1)
        val start  = System.currentTimeMillis()

        while (System.currentTimeMillis() - start < timeoutMs) {
            val available = try { input.available() } catch (e: IOException) { break }
            if (available > 0) {
                val n = try { input.read(buf) } catch (e: IOException) { break }
                if (n > 0) {
                    sb.append(buf[0].toInt().and(0xFF).toChar())
                    if (OBDParser.isPromptReady(sb.toString())) break
                }
            } else {
                Thread.sleep(5)
            }
        }
        return sb.toString()
    }

    private fun closeStreams() {
        try { inputStream?.close()  } catch (_: IOException) {}
        try { outputStream?.close() } catch (_: IOException) {}
        try { socket?.close()       } catch (_: IOException) {}
        socket       = null
        inputStream  = null
        outputStream = null
    }
}
