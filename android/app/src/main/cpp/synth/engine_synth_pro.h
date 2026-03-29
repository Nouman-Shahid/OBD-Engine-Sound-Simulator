#pragma once

#include <cmath>
#include <cstdint>
#include <atomic>
#include "../dsp/biquad_filter.h"
#include "../dsp/comb_filter.h"

namespace enginex {

// ── Constants ────────────────────────────────────────────────────────────────

static constexpr int   kProMaxHarmonics = 12;
static constexpr int   kProMaxCylinders = 12;
static constexpr float kProTwoPi        = 6.28318530717958647f;
static constexpr float kProSampleRate   = 48000.0f;
static constexpr int   kCombDelayMax    = 4096;  // ~10 Hz minimum firing freq
static constexpr int   kBackfireLength  = 8192;  // ~170 ms burst

/**
 * Professional-grade real-time engine sound synthesiser.
 *
 * Synthesis model (executed per sample in render()):
 *
 *  ┌─────────────────────────────────────────────────────────────────────┐
 *  │  1.  CYLINDER FIRING PULSES                                         │
 *  │      Each cylinder triggers a decaying sinusoidal pulse at TDC.    │
 *  │      Cylinders are evenly phased around [0, 2π).                   │
 *  │      This creates the characteristic engine "chug" that pure        │
 *  │      additive synthesis misses.                                     │
 *  │                                                                     │
 *  │  2.  ADDITIVE HARMONIC LAYER                                        │
 *  │      8–12 harmonics weighted per vehicle profile.                   │
 *  │      Upper harmonics bloom with RPM (harmonic emphasis).            │
 *  │      Anti-aliased (Nyquist guard per harmonic).                     │
 *  │                                                                     │
 *  │  3.  MIX: pulses × α + harmonics × (1-α)                           │
 *  │                                                                     │
 *  │  4.  SOFT SATURATION (cubic)                                        │
 *  │      Increases with RPM² — models exhaust harmonic compression.     │
 *  │                                                                     │
 *  │  5.  VARIABLE COMB FILTER (exhaust resonance)                       │
 *  │      Delay tracks Fs / (2·f₀). Positive feedback = resonance.      │
 *  │      Models the standing wave in the exhaust pipe.                  │
 *  │                                                                     │
 *  │  6.  FORMANT FILTER BANK (2 × Biquad bandpass)                     │
 *  │      Low formant  ≈ 200–500 Hz  (body resonance)                   │
 *  │      High formant ≈ 600–2500 Hz (intake / exhaust character)        │
 *  │      Both centre frequencies can be modulated by profile.           │
 *  │                                                                     │
 *  │  7.  INTAKE NOISE (bandpass-filtered LFSR noise)                    │
 *  │      Throttle-dependent induction roar.                             │
 *  │                                                                     │
 *  │  8.  TURBO WHINE (optional)                                         │
 *  │      Compressor whine: f = RPM/60 × speedRatio × bladeCount.       │
 *  │      Boost envelope lags throttle × RPM.                            │
 *  │                                                                     │
 *  │  9.  BACKFIRE / POP (transient, throttle-lift triggered)            │
 *  │      Bandpass-filtered noise burst with exponential decay.          │
 *  │      Auto-triggered when throttle drops from > 0.65 to < 0.15      │
 *  │      while RPM > 2500.                                              │
 *  │                                                                     │
 *  │  10. VCA + one-pole anti-click LPF                                  │
 *  │  11. Hard-clip safety net                                           │
 *  └─────────────────────────────────────────────────────────────────────┘
 *
 * Real-time safety:
 *  - No heap allocation anywhere.
 *  - All cross-thread parameters use std::atomic<float/int>.
 *  - Atomics snapshotted once at start of each buffer.
 *  - Filter coefficients updated lazily only when frequency changes > 2 Hz.
 *  - Galois LFSR noise: deterministic and lock-free.
 */
class EngineSynthPro {
public:
    EngineSynthPro();

    // ── Cross-thread parameters (written from Kotlin, read from audio CB) ────

    std::atomic<float> targetRpm       {850.0f};
    std::atomic<float> targetThrottle  {0.0f};
    std::atomic<int>   cylinders       {4};

    /** Per-harmonic weights [h0..h11]. h0 = fundamental. */
    std::atomic<float> harmonicWeights[kProMaxHarmonics];

    /** Noise level for intake breath [0, 1]. */
    std::atomic<float> noiseLevel      {0.08f};

    // ── Turbo ────────────────────────────────────────────────────────────────
    std::atomic<float> turboGain       {0.0f};   ///< 0 = no turbo
    std::atomic<float> turboSpeedRatio {14.0f};  ///< turbo RPM / engine RPM
    std::atomic<int>   turboBladeCount {11};     ///< compressor blade count

    // ── Formant filter ───────────────────────────────────────────────────────
    std::atomic<float> formantFreq0    {320.0f};
    std::atomic<float> formantFreq1    {1200.0f};
    std::atomic<float> formantQ0       {2.5f};
    std::atomic<float> formantQ1       {3.0f};
    std::atomic<float> formantGain0    {0.35f};
    std::atomic<float> formantGain1    {0.25f};

    // ── Comb (exhaust) ───────────────────────────────────────────────────────
    std::atomic<float> combFeedback    {0.28f};

    // ── Backfire trigger (set from Kotlin, cleared inside render) ────────────
    std::atomic<bool>  triggerBackfire {false};

    // ── Audio callback interface ─────────────────────────────────────────────

    /** Fill [buffer] with [numFrames] mono float32 samples. Lock-free. */
    void render(float* buffer, int32_t numFrames) noexcept;

    /** Reset all state — call when engine stops. */
    void reset() noexcept;

private:
    // ── Per-render smoothed state (audio-thread owned) ───────────────────────
    float _smoothRpm      {850.0f};
    float _smoothThrottle {0.0f};
    float _prevThrottle   {0.0f};   // for backfire detection
    float _turboBoost     {0.0f};
    float _volLpfState    {0.0f};

    // Cylinder firing phases and pulse amplitudes
    float _cylPhases [kProMaxCylinders]{};
    float _pulseAmps [kProMaxCylinders]{};

    // Harmonic phase accumulators
    float _harmPhases[kProMaxHarmonics]{};

    // DSP units
    CombFilter<kCombDelayMax> _comb;
    BiquadFilter _formant0;
    BiquadFilter _formant1;
    BiquadFilter _intakeFilter;
    BiquadFilter _backfireFilter;

    // Cache to skip coefficient recalculation each sample
    float _cachedFormantFreq0 {0.0f};
    float _cachedFormantFreq1 {0.0f};
    float _cachedIntakeFreq   {0.0f};

    // Turbo oscillator
    float _turboPhase{0.0f};

    // Backfire burst state
    int   _backfireCounter{0};
    float _backfireLevel  {0.0f};

    // Volume anti-click LPF coefficient (80 Hz cutoff)
    const float _volLpfCoeff;

    // Galois LFSR noise
    uint32_t _noiseState{0xF1E2D3C4u};
    float nextNoise() noexcept;

    // Smoothing constants (per-sample exponential approach)
    static constexpr float kRpmSmooth       = 0.0018f;  // ~2 RPM/sample
    static constexpr float kThrottleSmooth  = 0.006f;
    static constexpr float kBoostSmooth     = 0.0008f;
    static constexpr float kPulseDecay      = 0.9994f;  // ~330 ms to -60 dB
    static constexpr float kBackfireDecay   = 0.9996f;
};

} // namespace enginex
