#include "oboe_engine.h"
#include <android/log.h>
#include <cstring>
#include <algorithm>

#define LOG_TAG "OboeEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)

namespace enginex {

OboeEngine::OboeEngine() = default;

OboeEngine::~OboeEngine() {
    release();
}

// ── Init ──────────────────────────────────────────────────────────────────────

bool OboeEngine::init(int    cylinders,
                      float  idleRpm,
                      float  /*revLimiterRpm*/,
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
                      int    turboBladeCount)
{
    _synth.cylinders .store(cylinders,  std::memory_order_relaxed);
    _synth.targetRpm .store(idleRpm,    std::memory_order_relaxed);
    _synth.noiseLevel.store(noiseLevel, std::memory_order_relaxed);
    _synth.combFeedback.store(combFeedback, std::memory_order_relaxed);
    _synth.formantFreq0.store(formantFreq0, std::memory_order_relaxed);
    _synth.formantFreq1.store(formantFreq1, std::memory_order_relaxed);
    _synth.formantQ0   .store(formantQ0,    std::memory_order_relaxed);
    _synth.formantQ1   .store(formantQ1,    std::memory_order_relaxed);
    _synth.formantGain0.store(formantGain0, std::memory_order_relaxed);
    _synth.formantGain1.store(formantGain1, std::memory_order_relaxed);
    _synth.turboGain       .store(turboGain,        std::memory_order_relaxed);
    _synth.turboSpeedRatio .store(turboSpeedRatio,  std::memory_order_relaxed);
    _synth.turboBladeCount .store(turboBladeCount,  std::memory_order_relaxed);

    const int count = std::min(numWeights, kProMaxHarmonics);
    for (int i = 0; i < count; ++i) {
        _synth.harmonicWeights[i].store(harmonicWeights[i], std::memory_order_relaxed);
    }
    // Zero remaining weights
    for (int i = count; i < kProMaxHarmonics; ++i) {
        _synth.harmonicWeights[i].store(0.0f, std::memory_order_relaxed);
    }

    return openStream();
}

// ── Start / Stop ──────────────────────────────────────────────────────────────

bool OboeEngine::start() {
    if (!_stream) {
        LOGE("start() called but stream is null");
        return false;
    }
    const auto result = _stream->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("requestStart failed: %s", oboe::convertToText(result));
        return false;
    }
    _running.store(true, std::memory_order_release);
    LOGI("Stream started. SR=%d ch=%d buf=%d latency~%lldms",
         _sampleRate, _channelCount,
         _stream->getBufferSizeInFrames(),
         (long long)(_stream->getBufferSizeInFrames() * 1000LL / _sampleRate));
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
    _synth.targetRpm     .store(rpm,      std::memory_order_relaxed);
    _synth.targetThrottle.store(throttle, std::memory_order_relaxed);
}

void OboeEngine::setProfile(int    cylinders,
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
                             int    turboBladeCount)
{
    _synth.cylinders   .store(cylinders,    std::memory_order_relaxed);
    _synth.noiseLevel  .store(noiseLevel,   std::memory_order_relaxed);
    _synth.combFeedback.store(combFeedback, std::memory_order_relaxed);
    _synth.formantFreq0.store(formantFreq0, std::memory_order_relaxed);
    _synth.formantFreq1.store(formantFreq1, std::memory_order_relaxed);
    _synth.formantQ0   .store(formantQ0,    std::memory_order_relaxed);
    _synth.formantQ1   .store(formantQ1,    std::memory_order_relaxed);
    _synth.formantGain0.store(formantGain0, std::memory_order_relaxed);
    _synth.formantGain1.store(formantGain1, std::memory_order_relaxed);
    _synth.turboGain      .store(turboGain,       std::memory_order_relaxed);
    _synth.turboSpeedRatio.store(turboSpeedRatio, std::memory_order_relaxed);
    _synth.turboBladeCount.store(turboBladeCount, std::memory_order_relaxed);

    const int count = std::min(numWeights, kProMaxHarmonics);
    for (int i = 0; i < count; ++i) {
        _synth.harmonicWeights[i].store(harmonicWeights[i], std::memory_order_relaxed);
    }
    for (int i = count; i < kProMaxHarmonics; ++i) {
        _synth.harmonicWeights[i].store(0.0f, std::memory_order_relaxed);
    }
}

// ── Audio callback (hot path — MUST be lock-free) ─────────────────────────────

oboe::DataCallbackResult OboeEngine::onAudioReady(
        oboe::AudioStream* /*stream*/,
        void*   audioData,
        int32_t numFrames) {

    auto* out = static_cast<float*>(audioData);

    if (!_running.load(std::memory_order_acquire)) {
        // Output silence when stopped
        std::memset(out, 0, sizeof(float) * static_cast<size_t>(numFrames) * _channelCount);
        return oboe::DataCallbackResult::Continue;
    }

    // Clamp to scratch buffer size
    const int32_t frames = std::min(numFrames, kMaxBurst);

    // Render mono into scratch buffer
    _synth.render(_monoScratch, frames);

    // Duplicate mono → stereo interleaved (L R L R …)
    for (int32_t f = 0; f < frames; ++f) {
        out[f * 2]     = _monoScratch[f];  // L
        out[f * 2 + 1] = _monoScratch[f];  // R
    }

    return oboe::DataCallbackResult::Continue;
}

// ── Error recovery ────────────────────────────────────────────────────────────

void OboeEngine::onErrorAfterClose(oboe::AudioStream* /*stream*/,
                                   oboe::Result       error) {
    LOGW("Stream error: %s — attempting restart", oboe::convertToText(error));
    if (_running.load()) {
        closeStream();
        if (openStream()) {
            start();
            LOGI("Stream restarted after error");
        } else {
            LOGE("Failed to reopen stream after error");
        }
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

bool OboeEngine::openStream() {
    closeStream();

    oboe::AudioStreamBuilder builder;
    builder.setDirection          (oboe::Direction::Output)
           .setPerformanceMode    (oboe::PerformanceMode::LowLatency)
           .setSharingMode        (oboe::SharingMode::Exclusive)
           .setFormat             (oboe::AudioFormat::Float)
           .setChannelCount       (oboe::ChannelCount::Stereo)
           .setSampleRate         (48000)
           .setDataCallback       (this)
           .setErrorCallback      (this)
           // 256-frame burst ≈ 5.3 ms at 48 kHz (stereo = 512 floats per cb)
           .setFramesPerDataCallback(256)
           .setBufferCapacityInFrames(512);

    const auto result = builder.openStream(_stream);
    if (result != oboe::Result::OK) {
        LOGE("openStream failed: %s", oboe::convertToText(result));
        _stream.reset();
        return false;
    }

    _sampleRate   = _stream->getSampleRate();
    _channelCount = _stream->getChannelCount();
    LOGI("Stream opened: SR=%d ch=%d frames/cb=%d sharing=%s",
         _sampleRate, _channelCount,
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
