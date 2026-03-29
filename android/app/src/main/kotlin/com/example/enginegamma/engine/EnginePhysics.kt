package com.example.enginegamma.engine

import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

/**
 * Simulates engine RPM physics on the Kotlin side.
 *
 * Updated at ~60 Hz by [EngineChannelHandler]. The resulting RPM
 * is forwarded to the C++ DSP via [OboeAudioEngine.setParams].
 *
 * Features:
 *  - Exponential RPM ramp controlled by [VehicleProfile.inertiaFactor]
 *  - Gear-dependent load resistance (higher gear = slower RPM rise)
 *  - Idle RPM hold when throttle = 0
 *  - Rev limiter cut (hard cap at revLimiterRpm)
 *  - OBD bypass: when OBD data is live, simulated physics are skipped
 *    and real RPM from the ECU is used directly.
 */
class EnginePhysics {

    // ── State ─────────────────────────────────────────────────────────────────

    var currentRpm:    Double = 0.0
        private set

    var isRevLimiting: Boolean = false
        private set

    // ── Configuration ─────────────────────────────────────────────────────────

    private var profile:    VehicleProfiles.VehicleProfile = VehicleProfiles.sport4
    private var throttle:   Double  = 0.0
    private var gear:       Int     = 0
    private var isRunning:  Boolean = false

    // OBD override
    private var obdRpm:     Double? = null   // non-null when OBD is connected
    private var obdThrottle: Double? = null

    // ── Public API ────────────────────────────────────────────────────────────

    fun setProfile(p: VehicleProfiles.VehicleProfile) {
        profile = p
        if (isRunning && currentRpm < p.idleRpm) currentRpm = p.idleRpm
    }

    fun setThrottle(t: Double) {
        throttle = t.coerceIn(0.0, 1.0)
    }

    fun setGear(g: Int) {
        gear = g.coerceIn(0, 6)
    }

    fun start() {
        isRunning  = true
        currentRpm = profile.idleRpm
    }

    fun stop() {
        isRunning  = false
        currentRpm = 0.0
        isRevLimiting = false
    }

    /** Feed live OBD RPM (overrides simulation while non-null). */
    fun setObdData(rpm: Double?, throttlePos: Double?) {
        obdRpm      = rpm
        obdThrottle = throttlePos
    }

    fun clearObdData() {
        obdRpm      = null
        obdThrottle = null
    }

    /**
     * Advance simulation by [deltaMs] milliseconds.
     * Returns effective (rpm, throttle) pair for DSP update.
     */
    fun update(deltaMs: Double): Pair<Double, Double> {
        if (!isRunning) return Pair(0.0, 0.0)

        val effectiveThrottle: Double

        // OBD bypass: real car data takes precedence
        if (obdRpm != null) {
            currentRpm        = obdRpm!!.coerceIn(0.0, profile.maxRpm)
            effectiveThrottle = (obdThrottle ?: throttle).coerceIn(0.0, 1.0)
            isRevLimiting     = currentRpm >= profile.revLimiterRpm
            return Pair(currentRpm, effectiveThrottle)
        }

        effectiveThrottle = throttle

        // Gear load factor: neutral = 0 load, 1st gear = high load, 6th = low
        val gearRatio  = if (gear > 0 && gear < profile.gearRatios.size)
                             profile.gearRatios[gear] else 1.0
        val loadFactor = if (gear > 0) 1.0 - (1.0 / (gearRatio + 0.5)) * 0.20 else 1.0

        // Target RPM
        val baseTarget  = profile.idleRpm + throttle * (profile.revLimiterRpm - profile.idleRpm)
        val targetRpm   = baseTarget * loadFactor

        // Exponential approach with inertia (scaled to 60 Hz tick)
        val k      = profile.inertiaFactor * (deltaMs / 16.667)
        val rpmErr = targetRpm - currentRpm
        currentRpm += rpmErr * k.coerceIn(0.0, 1.0)

        // Rev limiter
        if (currentRpm >= profile.revLimiterRpm) {
            currentRpm    = profile.revLimiterRpm
            isRevLimiting = true
        } else {
            isRevLimiting = false
        }

        // Floor at idle when engine is running and throttle released
        if (throttle < 0.02 && currentRpm < profile.idleRpm) {
            currentRpm += (profile.idleRpm - currentRpm) * 0.04
        }

        currentRpm = max(0.0, currentRpm)
        return Pair(currentRpm, effectiveThrottle)
    }

    /** Returns the active audio zone label for the Flutter event stream. */
    fun getActiveLayer(): String {
        val n = currentRpm / profile.maxRpm
        return when {
            n < 0.25 -> "idle"
            n < 0.50 -> "low"
            n < 0.75 -> "mid"
            else     -> "high"
        }
    }

    /** True if a rapid throttle lift at high RPM just occurred (backfire candidate). */
    private var prevThrottle = 0.0
    fun checkBackfire(): Boolean {
        val backfire = prevThrottle > 0.65 && throttle < 0.15 && currentRpm > 2500
        prevThrottle = throttle
        return backfire
    }
}
