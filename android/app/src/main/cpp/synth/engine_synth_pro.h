#pragma once

#include <cmath>
#include <cstdint>
#include <atomic>
#include "../dsp/biquad_filter.h"
#include "../dsp/comb_filter.h"
#include "../audio/audio_sample.h"
#include "../audio/sample_player.h"
#include "../audio/procedural_samples.h"

namespace enginex {

// ── Constants ────────────────────────────────────────────────────────────────

static constexpr int   kProMaxHarmonics = 12;
static constexpr int   kProMaxCylinders = 12;
static constexpr float kProTwoPi        = 6.28318530717958647f;
static constexpr float kProSampleRate   = 48000.0f;
static constexpr int   kCombDelayMax    = 4096;  // ~10 Hz minimum firing freq
static constexpr int   kBackfireLength  = 8192;  // ~170 ms burst

/**
 * Professional-grade real-time engine sound synthesiser — Phase 2.
 *
 * Synthesis model (executed per sample in render()):
 *
 *  ┌─────────────────────────────────────────────────────────────────────┐
 *  │  1.  CYLINDER FIRING PULSES                                         │
 *  │      Each cylinder triggers a decaying sinusoidal pulse at TDC.    │
 *  │      Phase jitter (~0.00005 rad/sample) adds organic variation.    │
 *  │                                                                     │
 *  │  2.  ADDITIVE HARMONIC LAYER                                        │
 *  │      12 harmonics weighted per vehicle profile.                     │
 *  │      Upper harmonics bloom progressively with RPM.                  │
 *  │                                                                     │
 *  │  3.  MIX: pulses × α + harmonics × (1-α)                           │
 *  │                                                                     │
 *  │  4.  SOFT SATURATION (cubic)                                        │
 *  │      Increases with RPM² + engine load — models exhaust             │
 *  │      harmonic compression under acceleration.                       │
 *  │                                                                     │
 *  │  5.  VARIABLE COMB FILTER (exhaust resonance)                       │
 *  │      Delay tracks Fs / (2·f₀). Feedback boosted under load         │
 *  │      to simulate standing-wave amplification in exhaust pipe.       │
 *  │                                                                     │
 *  │  6.  FORMANT FILTER BANK (2 × Biquad bandpass)                     │
 *  │      Low formant  ≈ 200–500 Hz  (body resonance)                   │
 *  │      High formant ≈ 600–2500 Hz (intake / exhaust character)        │
 *  │                                                                     │
 *  │  7.  EXHAUST TEXTURE SAMPLE LAYER (new in Phase 2)                  │
 *  │      Bandpass-noise sample looped at variable speed, mixed under    │
 *  │      load. Adds grit that pure synthesis lacks.                     │
 *  │                                                                     │
 *  │  8.  INTAKE NOISE (bandpass-filtered LFSR + sample layer)           │
 *  │      LFSR for rapid variation; sample layer for organic texture.    │
 *  │      Throttle-tracked cutoff and amplitude.                         │
 *  │                                                                     │
 *  │  9.  TURBO WHINE + TEXTURE (optional)                               │
 *  │      Whine oscillator (f = RPM/60 × speedRatio × bladeCount) plus  │
 *  │      high-frequency spool noise sample.                             │
 *  │                                                                     │
 *  │  10. BACKFIRE / POP (transient, throttle-lift or manual)            │
 *  │      LFSR bandpass burst + backfire sample layer.                   │
 *  │                                                                     │
 *  │  11. OUTPUT LOW-PASS FILTER (throttle-dependent brightness)         │
 *  │      Cutoff = 1500 + throttle × 7500 Hz.                           │
 *  │      Dark and muted at idle; aggressive and open at WOT.            │
 *  │      Coefficient computed once per buffer (not per sample).         │
 *  │                                                                     │
 *  │  12. VCA + one-pole anti-click LPF                                  │
 *  │  13. Hard-clip safety net                                           │
 *  └─────────────────────────────────────────────────────────────────────┘
 *
 * Real-time safety:
 *  - No heap allocation anywhere.
 *  - All cross-thread parameters use std::atomic<float/int>.
 *  - Atomics snapshotted once at start of each buffer.
 *  - Filter coefficients updated lazily only when frequency changes > 2 Hz.
 *  - Sample players use static BSS arrays (ProceduralSamples).
 *  - Galois LFSR noise: deterministic and lock-free.
 */
class EngineSynthPro {
public:
    EngineSynthPro();

    // ── Cross-thread parameters (written from Kotlin, read from audio CB) ────

    std::atomic<float> targetRpm       {850.0f};
    std::atomic<float> targetThrottle  {0.0f};
    std::atomic<int>   cylinders       {4};

    /** Current gear [0 = neutral/unspecified, 1–6]. Affects engine load calc. */
    std::atomic<int>   gear            {0};

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

    // ── Hybrid sample mixing gains (per vehicle profile) ────────────────────
    std::atomic<float> intakeSampleGain  {0.06f};  ///< Intake sample blend [0,1]
    std::atomic<float> exhaustSampleGain {0.08f};  ///< Exhaust texture blend [0,1]

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
    float _outLpfState    {0.0f};   // throttle-dependent output brightness LPF

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

    // ── Hybrid sample players ─────────────────────────────────────────────────
    SamplePlayer _intakePlayer;
    SamplePlayer _exhaustPlayer;
    SamplePlayer _turboPlayer;
    SamplePlayer _backfirePlayer;

    // Galois LFSR noise
    uint32_t _noiseState{0xF1E2D3C4u};
    float nextNoise() noexcept;

    // ── Smoothing constants (per-sample exponential approach) ─────────────────
    static constexpr float kRpmSmooth       = 0.0018f;  // ~2 RPM/sample
    static constexpr float kThrottleSmooth  = 0.006f;
    static constexpr float kBoostSmooth     = 0.0008f;
    static constexpr float kPulseDecay      = 0.9994f;  // ~330 ms to -60 dB
    static constexpr float kBackfireDecay   = 0.9996f;

    // ── Phase jitter — organic cylinder timing variation ──────────────────────
    // Adds ~0.003% of a cycle per sample: just enough for organic feel
    static constexpr float kJitterScale     = 0.00005f;
};

} // namespace enginex
