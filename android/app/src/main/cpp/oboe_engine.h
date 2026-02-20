#pragma once

#include <oboe/Oboe.h>
#include <atomic>
#include <memory>
#include "synth/engine_synth.h"

namespace enginex {

/**
 * Low-latency Oboe audio stream + engine synthesiser.
 *
 * Oboe is asked for AAUDIO_PERFORMANCE_MODE_LOW_LATENCY with EXCLUSIVE
 * sharing mode. On devices that can't satisfy EXCLUSIVE it automatically
 * falls back to SHARED while preserving low-latency mode.
 *
 * The audio callback (onAudioReady) is called by Oboe on a high-priority
 * real-time thread. We fill the buffer by calling EngineSynth::render —
 * entirely lock-free.
 *
 * Error recovery: if the stream is disconnected (e.g. headphone unplug)
 * onErrorAfterClose restarts the stream automatically.
 */
class OboeEngine : public oboe::AudioStreamDataCallback,
                   public oboe::AudioStreamErrorCallback {
public:
    OboeEngine();
    ~OboeEngine();

    bool init(int cylinders, float idleRpm, float revLimiterRpm,
              const float* harmonicWeights, int numWeights, float noiseLevel);

    bool start();
    void stop();
    void release();

    void setParams(float rpm, float throttle);
    void setProfile(int cylinders, const float* harmonicWeights,
                    int numWeights, float noiseLevel);

    bool isRunning() const { return _running.load(std::memory_order_relaxed); }

    // ── AudioStreamDataCallback ───────────────────────────────────────────────
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream* stream,
        void*              audioData,
        int32_t            numFrames) override;

    // ── AudioStreamErrorCallback ──────────────────────────────────────────────
    void onErrorAfterClose(oboe::AudioStream* stream,
                           oboe::Result       error) override;

private:
    std::shared_ptr<oboe::AudioStream> _stream;
    EngineSynth  _synth;
    std::atomic<bool> _running{false};
    int _sampleRate{48000};

    bool openStream();
    void closeStream();
};

} // namespace enginex
