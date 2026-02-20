#include "engine_synth.h"
#include <cstring>

namespace enginex {

// ── Constructor ────────────────────────────────────────────────────────────

EngineSynth::EngineSynth()
    : _volLpfCoeff(std::exp(-kTwoPi * 80.0f / kSampleRate))
{
    // Default: sport 4-cylinder harmonic weights.
    float defaults[kMaxHarmonics] = {1.0f, 0.6f, 0.35f, 0.18f, 0.10f, 0.06f, 0.04f, 0.02f};
    for (int h = 0; h < kMaxHarmonics; ++h) {
        harmonicWeights[h].store(defaults[h], std::memory_order_relaxed);
    }
    reset();
}

// ── Reset ──────────────────────────────────────────────────────────────────

void EngineSynth::reset() {
    std::memset(_phases, 0, sizeof(_phases));
    _smoothRpm       = targetRpm.load(std::memory_order_relaxed);
    _smoothThrottle  = 0.0f;
    _smoothVolume    = 0.0f;
    _volLpfState     = 0.0f;
    _noiseState      = 0xDEADBEEF;
}

// ── Audio render ───────────────────────────────────────────────────────────

/**
 * Fills [numFrames] of [buffer] with mono PCM float32 engine audio.
 *
 * Per-sample algorithm:
 *  1.  Exponentially smooth RPM and throttle toward their atomic targets.
 *  2.  Compute firing frequency f0 = RPM/60 × (cyl/2).
 *  3.  For each harmonic h ∈ [1..N]:
 *        freq_h = (h+1) × f0
 *        Guard: only synthesise if freq_h < Nyquist (Fs/2).
 *        Advance phase: φ_h += 2π × freq_h / Fs
 *        s += w[h] × sin(φ_h)
 *  4.  Normalise by sum of active weights.
 *  5.  Layer blend: mix in RPM-zone noise / brightness boost.
 *  6.  Add band-limited noise component.
 *  7.  Apply volume (one-pole anti-click LPF on volume).
 *  8.  Hard-clip to [-1, 1] as safety net.
 */
void EngineSynth::render(float* buffer, int32_t numFrames) {

    // Snapshot atomics once per buffer (not per sample).
    const float tgtRpm      = targetRpm     .load(std::memory_order_relaxed);
    const float tgtThrottle = targetThrottle.load(std::memory_order_relaxed);
    const int   cyl         = cylinders     .load(std::memory_order_relaxed);
    const float noiseLvl    = noiseLevel    .load(std::memory_order_relaxed);

    // Snapshot harmonic weights once per buffer.
    float hw[kMaxHarmonics];
    for (int h = 0; h < kMaxHarmonics; ++h) {
        hw[h] = harmonicWeights[h].load(std::memory_order_relaxed);
    }

    const float nyquist = kSampleRate * 0.5f;

    for (int32_t i = 0; i < numFrames; ++i) {

        // ── 1. Smooth RPM and throttle ────────────────────────────────────
        _smoothRpm      += (_smoothRpm      < tgtRpm      ? 1.0f : -1.0f)
                           * std::min(kRpmSmooth * std::abs(tgtRpm - _smoothRpm),
                                      std::abs(tgtRpm - _smoothRpm));
        // Simpler per-sample exponential approach:
        _smoothRpm      += (tgtRpm      - _smoothRpm)      * kRpmSmooth;
        _smoothThrottle += (tgtThrottle - _smoothThrottle) * kThrottleSmooth;

        // ── 2. Firing frequency ───────────────────────────────────────────
        const float f0 = (_smoothRpm / 60.0f) * (static_cast<float>(cyl) * 0.5f);

        // ── 3. Sum harmonics ──────────────────────────────────────────────
        float sample     = 0.0f;
        float totalWeight = 0.0f;

        for (int h = 0; h < kMaxHarmonics; ++h) {
            if (hw[h] < 0.001f) continue;

            const float freq = f0 * static_cast<float>(h + 1);
            if (freq >= nyquist || freq < 1.0f) continue;  // anti-alias guard

            const float dPhase = kTwoPi * freq / kSampleRate;
            _phases[h] += dPhase;
            if (_phases[h] >= kTwoPi) _phases[h] -= kTwoPi;

            // Layer-dependent amplitude: higher harmonics bloom at high RPM.
            const float normalRpm = _smoothRpm / 8000.0f;  // normalise to 8k RPM
            // Harmonic emphasis factor: upper harmonics gain weight with RPM.
            const float harmEmphasis = 1.0f + normalRpm * 0.5f * static_cast<float>(h);
            const float amp = hw[h] * harmEmphasis;

            sample      += amp * std::sin(_phases[h]);
            totalWeight += amp;
        }

        // ── 4. Normalise ──────────────────────────────────────────────────
        if (totalWeight > 0.001f) sample /= totalWeight;

        // ── 5. Layer blend brightness: add even-order distortion at high RPM
        {
            const float normalRpm = _smoothRpm / 8000.0f;
            // Soft saturation that increases with RPM — simulates exhaust harmonics.
            const float satAmt = normalRpm * normalRpm * 0.3f;
            sample = sample - satAmt * sample * sample * sample;  // cubic soft-clip
        }

        // ── 6. Noise component ────────────────────────────────────────────
        if (noiseLvl > 0.001f) {
            sample += noiseLvl * nextNoise();
        }

        // ── 7. Volume envelope ────────────────────────────────────────────
        // Base volume: idle hum present even at 0 throttle when engine is on.
        const float targetVol = (_smoothRpm < 10.0f) ? 0.0f
                              : 0.35f + _smoothThrottle * 0.65f;

        // One-pole LPF on volume for de-clicking.
        _volLpfState = targetVol + _volLpfCoeff * (_volLpfState - targetVol);
        sample *= _volLpfState;

        // ── 8. Safety hard-clip ───────────────────────────────────────────
        buffer[i] = std::fmax(-1.0f, std::fmin(1.0f, sample));
    }
}

// ── Helpers ────────────────────────────────────────────────────────────────

float EngineSynth::nextNoise() {
    // Galois LFSR — fast, deterministic, lock-free.
    _noiseState ^= _noiseState << 13;
    _noiseState ^= _noiseState >> 17;
    _noiseState ^= _noiseState << 5;
    return static_cast<float>(_noiseState) / static_cast<float>(0x7FFFFFFFu) - 1.0f;
}

float EngineSynth::smoothstep(float edge0, float edge1, float x) {
    x = std::fmax(0.0f, std::fmin(1.0f, (x - edge0) / (edge1 - edge0)));
    return x * x * (3.0f - 2.0f * x);
}

float EngineSynth::layerWeight(float normalizedRpm, float center, float halfWidth) {
    return smoothstep(center - halfWidth, center, normalizedRpm) *
           smoothstep(center + halfWidth, center, normalizedRpm);
}

} // namespace enginex
