#pragma once

#include <oboe/Oboe.h>
#include <atomic>
#include <memory>
#include "synth/engine_synth_pro.h"

namespace enginex {

/**
 * Low-latency stereo Oboe audio stream driving the pro engine synthesiser.
 *
 * Oboe configuration:
 *   - Direction:        Output only
 *   - PerformanceMode:  LowLatency
 *   - SharingMode:      Exclusive (falls back to Shared automatically)
 *   - Format:           Float32
 *   - Channels:         Stereo (L+R identical — mono synth, duplicated)
 *   - SampleRate:       48 000 Hz
 *   - FramesPerCallback:256 (≈5.3 ms)
 *
 * Error recovery:
 *   onErrorAfterClose() restarts the stream automatically on device
 *   disconnection (headphone unplug, Bluetooth source change, etc.).
 *
 * Thread safety:
 *   All parameters cross the thread boundary via std::atomic in EngineSynthPro.
 *   The audio callback is fully lock-free.
 */
class OboeEngine : public oboe::AudioStreamDataCallback,
                   public oboe::AudioStreamErrorCallback {
public:
    OboeEngine();
    ~OboeEngine();

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * Initialise synth and open Oboe stream.
     * @param harmonicWeights  Array of kProMaxHarmonics floats
     * @param numWeights       Length of harmonicWeights (≤ kProMaxHarmonics)
     */
    bool init(int    cylinders,
              float  idleRpm,
              float  revLimiterRpm,
              const float* harmonicWeights,
              int    numWeights,
              float  noiseLevel,
              float  combFeedback,
              float  formantFreq0,
              float  formantFreq1,
              float  formantQ0,
              float  formantQ1,
              float  formantGain0,
              float  formantGain1,
              float  turboGain,
              float  turboSpeedRatio,
              int    turboBladeCount);

    bool start();
    void stop();
    void release();

    // ── Real-time parameter updates ───────────────────────────────────────────

    /** Update RPM and throttle — called at ~60 Hz from Kotlin. */
    void setParams(float rpm, float throttle);

    /** Swap to a new vehicle profile. */
    void setProfile(int    cylinders,
                    const float* harmonicWeights,
                    int    numWeights,
                    float  noiseLevel,
                    float  combFeedback,
                    float  formantFreq0,
                    float  formantFreq1,
                    float  formantQ0,
                    float  formantQ1,
                    float  formantGain0,
                    float  formantGain1,
                    float  turboGain,
                    float  turboSpeedRatio,
                    int    turboBladeCount,
                    float  intakeSampleGain,
                    float  exhaustSampleGain);

    /** Update current gear [0 = neutral, 1–6]. Affects load DSP model. */
    void setGear(int gear) {
        _synth.gear.store(gear, std::memory_order_relaxed);
    }

    /** Manually trigger a backfire/pop event. */
    void triggerBackfire() {
        _synth.triggerBackfire.store(true, std::memory_order_relaxed);
    }

    bool isRunning() const {
        return _running.load(std::memory_order_relaxed);
    }

    // ── Oboe callbacks ────────────────────────────────────────────────────────

    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream* stream,
        void*              audioData,
        int32_t            numFrames) override;

    void onErrorAfterClose(oboe::AudioStream* stream,
                           oboe::Result       error) override;

private:
    std::shared_ptr<oboe::AudioStream> _stream;
    EngineSynthPro  _synth;
    std::atomic<bool> _running{false};
    int  _sampleRate  {48000};
    int  _channelCount{2};      // stereo output

    // Mono render scratch buffer (we render mono then duplicate to stereo)
    static constexpr int kMaxBurst = 512;
    float _monoScratch[kMaxBurst]{};

    bool openStream();
    void closeStream();
};

} // namespace enginex
