package com.enginex.audio

/**
 * Kotlin wrapper around the native C++ [OboeEngine].
 *
 * The C++ instance is kept alive as a raw pointer stored in [_nativeHandle].
 * JNI functions are named to match this package + class name:
 *   Java_com_enginex_audio_OboeAudioEngine_native*
 *
 * Thread safety:
 *   All parameter updates (setParams, setProfile) are safe to call from any
 *   thread — the C++ side uses lock-free atomics internally.
 *
 *   init() / release() should be called from the main thread at
 *   Activity create/destroy lifecycle events.
 */
class OboeAudioEngine {

    /** Holds the C++ OboeEngine* as a primitive long. */
    private var _nativeHandle: Long = 0L

    // ── Lifecycle ──────────────────────────────────────────────────────────────

    /**
     * Initialise the C++ engine with the given profile and open the Oboe stream.
     * Does NOT start playback — call [start] after this.
     */
    fun init(
        cylinders:       Int,
        idleRpm:         Double,
        revLimiterRpm:   Double,
        harmonicWeights: DoubleArray,
        noiseLevel:      Double,
        combFeedback:    Double,
        formantFreq0:    Double,
        formantFreq1:    Double,
        formantQ0:       Double,
        formantQ1:       Double,
        formantGain0:    Double,
        formantGain1:    Double,
        turboGain:       Double,
        turboSpeedRatio: Double,
        turboBladeCount: Int
    ): Boolean = nativeInit(
        cylinders, idleRpm, revLimiterRpm, harmonicWeights, noiseLevel,
        combFeedback,
        formantFreq0, formantFreq1, formantQ0, formantQ1, formantGain0, formantGain1,
        turboGain, turboSpeedRatio, turboBladeCount
    )

    /** Start audio playback. Must be called after [init]. */
    fun start(): Boolean = nativeStart()

    /** Stop audio playback and silence output. Keeps stream open. */
    fun stop() = nativeStop()

    /** Release all native resources. Do not use after this. */
    fun release() = nativeRelease()

    // ── Real-time controls ────────────────────────────────────────────────────

    /**
     * Update RPM and throttle — called at ~60 Hz by [EngineChannelHandler].
     * Lock-free, can be called from any thread.
     */
    fun setParams(rpm: Double, throttle: Double) =
        nativeSetParams(rpm, throttle)

    /**
     * Change to a different vehicle profile.
     * Takes effect within the next audio callback (~5 ms).
     */
    fun setProfile(
        cylinders:       Int,
        harmonicWeights: DoubleArray,
        noiseLevel:      Double,
        combFeedback:    Double,
        formantFreq0:    Double,
        formantFreq1:    Double,
        formantQ0:       Double,
        formantQ1:       Double,
        formantGain0:    Double,
        formantGain1:    Double,
        turboGain:       Double,
        turboSpeedRatio: Double,
        turboBladeCount: Int
    ) = nativeSetProfile(
        cylinders, harmonicWeights, noiseLevel,
        combFeedback,
        formantFreq0, formantFreq1, formantQ0, formantQ1, formantGain0, formantGain1,
        turboGain, turboSpeedRatio, turboBladeCount
    )

    /** Manually fire a backfire/pop event (e.g. on rapid throttle lift). */
    fun triggerBackfire() = nativeTriggerBackfire()

    /** @return true if the audio stream is actively rendering. */
    fun isRunning(): Boolean = nativeIsRunning()

    // ── Native declarations ───────────────────────────────────────────────────

    private external fun nativeInit(
        cylinders:       Int,
        idleRpm:         Double,
        revLimiterRpm:   Double,
        harmonicWeights: DoubleArray,
        noiseLevel:      Double,
        combFeedback:    Double,
        formantFreq0:    Double,
        formantFreq1:    Double,
        formantQ0:       Double,
        formantQ1:       Double,
        formantGain0:    Double,
        formantGain1:    Double,
        turboGain:       Double,
        turboSpeedRatio: Double,
        turboBladeCount: Int
    ): Boolean

    private external fun nativeStart(): Boolean
    private external fun nativeStop()
    private external fun nativeRelease()
    private external fun nativeSetParams(rpm: Double, throttle: Double)
    private external fun nativeSetProfile(
        cylinders:       Int,
        harmonicWeights: DoubleArray,
        noiseLevel:      Double,
        combFeedback:    Double,
        formantFreq0:    Double,
        formantFreq1:    Double,
        formantQ0:       Double,
        formantQ1:       Double,
        formantGain0:    Double,
        formantGain1:    Double,
        turboGain:       Double,
        turboSpeedRatio: Double,
        turboBladeCount: Int
    )
    private external fun nativeTriggerBackfire()
    private external fun nativeIsRunning(): Boolean

    companion object {
        init {
            System.loadLibrary("enginex_audio")
        }
    }
}
