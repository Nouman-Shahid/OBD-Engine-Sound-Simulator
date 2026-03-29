package com.example.enginegamma.engine

/**
 * Vehicle profile catalogue — Kotlin mirror of the Dart [VehicleProfile] models.
 *
 * These are used by [EnginePhysics] (RPM simulation) and passed to [OboeAudioEngine]
 * (DSP configuration) when a vehicle is selected.
 *
 * Profiles are designed so that harmonic weights + formant freqs + comb feedback
 * produce a recognisably different sonic character per vehicle.
 */
object VehicleProfiles {

    data class VehicleProfile(
        val id:              String,
        val name:            String,
        val cylinders:       Int,
        val idleRpm:         Double,
        val maxRpm:          Double,
        val revLimiterRpm:   Double,
        val inertiaFactor:   Double,    // RPM ramp speed [0..1]
        val gearRatios:      List<Double>,

        // ── DSP parameters ────────────────────────────────────────────────────
        /** Per-harmonic amplitude weights [h0..h11]. */
        val harmonicWeights: DoubleArray,
        /** Intake breath noise level [0, 1]. */
        val noiseLevel:      Double,
        /** Exhaust comb filter feedback — resonance strength [0, 0.5]. */
        val combFeedback:    Double,
        /** Low formant centre frequency (Hz) — body resonance. */
        val formantFreq0:    Double,
        /** High formant centre frequency (Hz) — intake/exhaust character. */
        val formantFreq1:    Double,
        val formantQ0:       Double,
        val formantQ1:       Double,
        val formantGain0:    Double,
        val formantGain1:    Double,

        // ── Turbo ─────────────────────────────────────────────────────────────
        /** Turbo whine amplitude [0 = no turbo]. */
        val turboGain:       Double = 0.0,
        /** Compressor speed / engine speed ratio (typ 12–16). */
        val turboSpeedRatio: Double = 14.0,
        /** Number of compressor blades (typ 9–13). */
        val turboBladeCount: Int    = 11,

        val emoji: String = "🚗"
    )

    // ── 2.0 L Sport 4-Cylinder ───────────────────────────────────────────────
    val sport4 = VehicleProfile(
        id            = "sport_4cyl",
        name          = "Sport 4-Cyl",
        cylinders     = 4,
        idleRpm       = 850.0,
        maxRpm        = 7200.0,
        revLimiterRpm = 7000.0,
        inertiaFactor = 0.08,
        gearRatios    = listOf(0.0, 3.36, 1.99, 1.33, 1.00, 0.82, 0.64),
        harmonicWeights = doubleArrayOf(1.0,0.60,0.35,0.18,0.10,0.06,0.04,0.02,0.01,0.005,0.002,0.001),
        noiseLevel    = 0.08,
        combFeedback  = 0.28,
        formantFreq0  = 310.0,
        formantFreq1  = 1150.0,
        formantQ0     = 2.5,
        formantQ1     = 3.0,
        formantGain0  = 0.35,
        formantGain1  = 0.25,
        emoji         = "🚗"
    )

    // ── 5.7 L Muscle V8 ──────────────────────────────────────────────────────
    val muscleV8 = VehicleProfile(
        id            = "muscle_v8",
        name          = "Muscle V8",
        cylinders     = 8,
        idleRpm       = 750.0,
        maxRpm        = 6500.0,
        revLimiterRpm = 6200.0,
        inertiaFactor = 0.05,
        gearRatios    = listOf(0.0, 2.97, 1.78, 1.26, 1.00, 0.84, 0.68),
        harmonicWeights = doubleArrayOf(1.0,0.80,0.50,0.30,0.16,0.10,0.06,0.04,0.02,0.01,0.005,0.002),
        noiseLevel    = 0.06,
        combFeedback  = 0.32,
        formantFreq0  = 180.0,  // deeper body resonance
        formantFreq1  = 750.0,
        formantQ0     = 2.0,
        formantQ1     = 2.8,
        formantGain0  = 0.42,
        formantGain1  = 0.30,
        emoji         = "🏎️"
    )

    // ── 3.0 L Inline-6 ───────────────────────────────────────────────────────
    val inline6 = VehicleProfile(
        id            = "inline_6",
        name          = "Inline-6",
        cylinders     = 6,
        idleRpm       = 800.0,
        maxRpm        = 7000.0,
        revLimiterRpm = 6800.0,
        inertiaFactor = 0.07,
        gearRatios    = listOf(0.0, 3.83, 2.20, 1.52, 1.22, 1.00, 0.81),
        harmonicWeights = doubleArrayOf(0.90,0.70,0.55,0.35,0.20,0.12,0.07,0.04,0.02,0.01,0.005,0.002),
        noiseLevel    = 0.05,
        combFeedback  = 0.25,
        formantFreq0  = 260.0,
        formantFreq1  = 1050.0,
        formantQ0     = 3.0,
        formantQ1     = 3.5,
        formantGain0  = 0.30,
        formantGain1  = 0.22,
        emoji         = "🚙"
    )

    // ── Single-Cylinder 450 cc ────────────────────────────────────────────────
    val singleCyl = VehicleProfile(
        id            = "single_cyl",
        name          = "Single Cyl Bike",
        cylinders     = 1,
        idleRpm       = 1200.0,
        maxRpm        = 9500.0,
        revLimiterRpm = 9200.0,
        inertiaFactor = 0.12,
        gearRatios    = listOf(0.0, 2.85, 1.80, 1.32, 1.04, 0.86, 0.70),
        harmonicWeights = doubleArrayOf(1.0,0.40,0.25,0.15,0.08,0.05,0.03,0.02,0.01,0.006,0.003,0.001),
        noiseLevel    = 0.12,
        combFeedback  = 0.22,
        formantFreq0  = 420.0,
        formantFreq1  = 1400.0,
        formantQ0     = 1.8,
        formantQ1     = 2.2,
        formantGain0  = 0.28,
        formantGain1  = 0.20,
        emoji         = "🏍️"
    )

    // ── 3.5 L V6 Bi-Turbo ────────────────────────────────────────────────────
    val v6Turbo = VehicleProfile(
        id            = "v6_turbo",
        name          = "V6 Bi-Turbo",
        cylinders     = 6,
        idleRpm       = 900.0,
        maxRpm        = 8000.0,
        revLimiterRpm = 7800.0,
        inertiaFactor = 0.09,
        gearRatios    = listOf(0.0, 3.55, 2.10, 1.45, 1.10, 0.88, 0.71),
        harmonicWeights = doubleArrayOf(0.85,0.75,0.60,0.40,0.25,0.14,0.08,0.05,0.03,0.015,0.008,0.003),
        noiseLevel    = 0.10,
        combFeedback  = 0.30,
        formantFreq0  = 290.0,
        formantFreq1  = 1100.0,
        formantQ0     = 2.8,
        formantQ1     = 3.2,
        formantGain0  = 0.33,
        formantGain1  = 0.27,
        turboGain       = 0.14,
        turboSpeedRatio = 15.0,
        turboBladeCount = 11,
        emoji = "⚡"
    )

    // ── 5.0 L Supercar V10 ────────────────────────────────────────────────────
    val supercardV10 = VehicleProfile(
        id            = "supercar_v10",
        name          = "Supercar V10",
        cylinders     = 10,
        idleRpm       = 800.0,
        maxRpm        = 9200.0,
        revLimiterRpm = 8800.0,
        inertiaFactor = 0.06,
        gearRatios    = listOf(0.0, 3.09, 2.19, 1.72, 1.36, 1.09, 0.87),
        harmonicWeights = doubleArrayOf(1.0,0.65,0.40,0.22,0.12,0.07,0.04,0.025,0.015,0.008,0.004,0.002),
        noiseLevel    = 0.07,
        combFeedback  = 0.34,
        formantFreq0  = 240.0,
        formantFreq1  = 980.0,
        formantQ0     = 3.2,
        formantQ1     = 4.0,
        formantGain0  = 0.38,
        formantGain1  = 0.30,
        emoji = "🔥"
    )

    // ── 4.0 L Flat-6 (Boxer) ─────────────────────────────────────────────────
    val flat6 = VehicleProfile(
        id            = "flat_6",
        name          = "Flat-6 Boxer",
        cylinders     = 6,
        idleRpm       = 900.0,
        maxRpm        = 8500.0,
        revLimiterRpm = 8200.0,
        inertiaFactor = 0.065,
        gearRatios    = listOf(0.0, 3.82, 2.26, 1.64, 1.28, 1.03, 0.84),
        harmonicWeights = doubleArrayOf(0.95,0.55,0.65,0.45,0.28,0.17,0.10,0.06,0.03,0.015,0.008,0.003),
        noiseLevel    = 0.09,
        combFeedback  = 0.26,
        formantFreq0  = 350.0,
        formantFreq1  = 1300.0,  // flat-6 has distinctive mid-high character
        formantQ0     = 2.2,
        formantQ1     = 2.8,
        formantGain0  = 0.32,
        formantGain1  = 0.26,
        emoji = "🏁"
    )

    // ── 2.0 L 4-Cyl Turbo (hot hatch) ────────────────────────────────────────
    val turbo4 = VehicleProfile(
        id            = "turbo_4cyl",
        name          = "4-Cyl Turbo",
        cylinders     = 4,
        idleRpm       = 900.0,
        maxRpm        = 7000.0,
        revLimiterRpm = 6800.0,
        inertiaFactor = 0.07,
        gearRatios    = listOf(0.0, 3.46, 1.95, 1.36, 1.03, 0.85, 0.70),
        harmonicWeights = doubleArrayOf(0.95,0.65,0.40,0.22,0.13,0.08,0.05,0.03,0.015,0.008,0.004,0.002),
        noiseLevel    = 0.09,
        combFeedback  = 0.27,
        formantFreq0  = 340.0,
        formantFreq1  = 1200.0,
        formantQ0     = 2.6,
        formantQ1     = 3.1,
        formantGain0  = 0.34,
        formantGain1  = 0.26,
        turboGain       = 0.11,
        turboSpeedRatio = 14.5,
        turboBladeCount = 10,
        emoji = "💨"
    )

    // ── Full catalogue ────────────────────────────────────────────────────────
    val all: List<VehicleProfile> = listOf(
        sport4, muscleV8, inline6, singleCyl,
        v6Turbo, supercardV10, flat6, turbo4
    )

    fun findById(id: String): VehicleProfile =
        all.firstOrNull { it.id == id } ?: sport4
}
