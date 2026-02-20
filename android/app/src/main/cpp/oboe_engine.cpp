#include "oboe_engine.h"
#include <android/log.h>
#include <cstring>

#define LOG_TAG "OboeEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)

namespace enginex {

OboeEngine::OboeEngine() = default;

OboeEngine::~OboeEngine() {
    release();
}

// ── Init ────────────────────────────────────────────────────────────────────

bool OboeEngine::init(int cylinders, float idleRpm, float revLimiterRpm,
                      const float* harmonicWeights, int numWeights,
                      float noiseLevel) {
    _synth.cylinders.store(cylinders, std::memory_order_relaxed);
    _synth.targetRpm.store(idleRpm,   std::memory_order_relaxed);
    _synth.noiseLevel.store(noiseLevel, std::memory_order_relaxed);

    int count = (numWeights < enginex::kMaxHarmonics)
              ? numWeights : enginex::kMaxHarmonics;
    for (int i = 0; i < count; ++i) {
        _synth.harmonicWeights[i].store(harmonicWeights[i],
                                        std::memory_order_relaxed);
    }

    return openStream();
}

// ── Start / Stop ─────────────────────────────────────────────────────────────

bool OboeEngine::start() {
    if (!_stream) {
        LOGE("start() called but stream is null");
        return false;
    }
    auto result = _stream->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("requestStart failed: %s", oboe::convertToText(result));
        return false;
    }
    _running.store(true, std::memory_order_release);
    LOGI("Stream started. SR=%d latency~%lldms",
         _sampleRate,
         (long long)(_stream->getBufferSizeInFrames() * 1000 / _sampleRate));
    return true;
}

void OboeEngine::stop() {
    _running.store(false, std::memory_order_release);
    if (_stream) {
        _stream->requestStop();
        _synth.reset();
    }
}

void OboeEngine::release() {
    stop();
    closeStream();
}

// ── Real-time parameter updates ───────────────────────────────────────────────

void OboeEngine::setParams(float rpm, float throttle) {
    _synth.targetRpm.store(rpm,      std::memory_order_relaxed);
    _synth.targetThrottle.store(throttle, std::memory_order_relaxed);
}

void OboeEngine::setProfile(int cylinders, const float* harmonicWeights,
                            int numWeights, float noiseLevel) {
    _synth.cylinders.store(cylinders,  std::memory_order_relaxed);
    _synth.noiseLevel.store(noiseLevel, std::memory_order_relaxed);
    int count = (numWeights < enginex::kMaxHarmonics)
              ? numWeights : enginex::kMaxHarmonics;
    for (int i = 0; i < count; ++i) {
        _synth.harmonicWeights[i].store(harmonicWeights[i],
                                        std::memory_order_relaxed);
    }
}

// ── Audio callback (hot path — MUST be lock-free) ─────────────────────────────

oboe::DataCallbackResult OboeEngine::onAudioReady(
        oboe::AudioStream* /*stream*/,
        void*   audioData,
        int32_t numFrames) {

    auto* out = static_cast<float*>(audioData);

    if (!_running.load(std::memory_order_acquire)) {
        // Engine stopped — output silence.
        std::memset(out, 0, sizeof(float) * static_cast<size_t>(numFrames));
        return oboe::DataCallbackResult::Continue;
    }

    _synth.render(out, numFrames);
    return oboe::DataCallbackResult::Continue;
}

// ── Error recovery ─────────────────────────────────────────────────────────────

void OboeEngine::onErrorAfterClose(oboe::AudioStream* /*stream*/,
                                   oboe::Result       error) {
    LOGW("Stream error: %s — attempting restart", oboe::convertToText(error));
    if (_running.load()) {
        closeStream();
        if (openStream()) {
            start();
            LOGI("Stream restarted after error");
        }
    }
}

// ── Private helpers ────────────────────────────────────────────────────────────

bool OboeEngine::openStream() {
    closeStream();

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
           .setPerformanceMode(oboe::PerformanceMode::LowLatency)
           .setSharingMode(oboe::SharingMode::Exclusive)
           .setFormat(oboe::AudioFormat::Float)
           .setChannelCount(oboe::ChannelCount::Mono)
           .setSampleRate(48000)
           .setDataCallback(this)
           .setErrorCallback(this)
           // Ask for a 256-frame burst — ~5.3 ms at 48 kHz.
           .setFramesPerDataCallback(256)
           // Minimal total buffer: 2× burst.
           .setBufferCapacityInFrames(512);

    auto result = builder.openStream(_stream);
    if (result != oboe::Result::OK) {
        LOGE("openStream failed: %s", oboe::convertToText(result));
        _stream.reset();
        return false;
    }

    _sampleRate = _stream->getSampleRate();
    LOGI("Stream opened: SR=%d frames/cb=%d sharing=%s",
         _sampleRate,
         _stream->getFramesPerDataCallback(),
         oboe::convertToText(_stream->getSharingMode()));
    return true;
}

void OboeEngine::closeStream() {
    if (_stream) {
        _stream->close();
        _stream.reset();
    }
}

} // namespace enginex
