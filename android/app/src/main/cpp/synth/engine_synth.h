#pragma once

#include <cmath>
#include <cstdint>
#include <array>
#include <atomic>

namespace enginex {

static constexpr int   kMaxHarmonics   = 8;
static constexpr float kTwoPi          = 2.0f * static_cast<float>(M_PI);
static constexpr float kSampleRate     = 48000.0f;

/**
 * Lock-free, real-time–safe engine sound synthesiser.
 *
 * Called exclusively from the Oboe audio callback (high-priority audio thread).
 * Parameters are updated from the Kotlin thread via std::atomic to avoid
 * any lock or allocation in the hot path.
 *
 * Synthesis model
 * ───────────────
 * The fundamental firing frequency of a 4-stroke engine is:
 *
 *   f₀ = RPM/60 × (cylinders/2)
 *
 * The output signal is a sum of harmonics:
 *
 *   s[n] = Σ w[h] × sin(2π × (h+1)×f₀ × n/Fs)
 *
 * plus a noise component for induction breath:
 *
 *   s[n] += noiseLevel × noise[n]
 *
 * Volume envelope:
 *   vol = idleGain + throttle × dynamicGain
 *
 * Anti-aliasing: Each harmonic is only synthesised while its frequency
 * is below the Nyquist limit (Fs/2). This prevents aliasing at high RPM.
 *
 * Layer crossfade: Four RPM zones (idle/low/mid/high) each have a weight
 * that is continuously blended from zero to one using cubic smoothstep.
 */
class EngineSynth {
public:
    EngineSynth();

    // ── Parameters (written from Kotlin, read from audio callback) ────────────

    /** Target RPM, smoothed inside render(). */
    std::atomic<float> targetRpm{850.0f};

    /** Throttle position [0, 1], smoothed inside render(). */
    std::atomic<float> targetThrottle{0.0f};

    /** Number of engine cylinders (controls firing frequency). */
    std::atomic<int>   cylinders{4};

    /** Per-harmonic weight [0,1] for harmonics 1..kMaxHarmonics. */
    std::atomic<float> harmonicWeights[kMaxHarmonics];

    /** Noise blend level [0, 1]. */
    std::atomic<float> noiseLevel{0.08f};

    // ── Audio callback interface ──────────────────────────────────────────────

    /**
     * Fill [buffer] with [numFrames] mono float32 samples.
     * This function is called on the audio thread — MUST be lock-free.
     */
    void render(float* buffer, int32_t numFrames);

    /** Reset all phase accumulators (call when engine stops). */
    void reset();

private:
    // ── Smoothed (per-render) state — audio-thread private ───────────────────
    float _smoothRpm      {850.0f};
    float _smoothThrottle {0.0f};
    float _smoothVolume   {0.0f};

    // Phase accumulators for each harmonic.
    float _phases[kMaxHarmonics]{};

    // One-pole low-pass for volume anti-click (80 Hz cutoff).
    float _volLpfState{0.0f};
    const float _volLpfCoeff;  // exp(-2π × 80 / Fs)

    // Exponential inertia coefficients (per-sample approach factor).
    static constexpr float kRpmSmooth      = 0.003f;  // ~1.5 RPM/sample convergence
    static constexpr float kThrottleSmooth = 0.005f;
    static constexpr float kVolSmooth      = 0.002f;

    // Noise PRNG (Galois LFSR — deterministic, lock-free).
    uint32_t _noiseState{0xDEADBEEF};
    inline float nextNoise();

    // Layer blend helper.
    static float smoothstep(float edge0, float edge1, float x);
    static float layerWeight(float normalizedRpm, float center, float halfWidth);
};

} // namespace enginex
