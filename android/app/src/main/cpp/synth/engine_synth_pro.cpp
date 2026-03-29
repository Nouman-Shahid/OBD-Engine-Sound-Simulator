#include "engine_synth_pro.h"

#include <cstring>
#include <algorithm>
#include <cmath>

namespace enginex {

// ── Constructor ───────────────────────────────────────────────────────────────

EngineSynthPro::EngineSynthPro()
    : _volLpfCoeff(std::exp(-kProTwoPi * 80.0f / kProSampleRate))
{
    // Default: sport 4-cylinder harmonic weights.
    const float defaults[kProMaxHarmonics] = {
        1.00f, 0.60f, 0.35f, 0.18f, 0.10f, 0.06f,
        0.04f, 0.02f, 0.01f, 0.006f, 0.003f, 0.001f
    };
    for (int h = 0; h < kProMaxHarmonics; ++h) {
        harmonicWeights[h].store(defaults[h], std::memory_order_relaxed);
    }
    reset();
}

// ── Reset ─────────────────────────────────────────────────────────────────────

void EngineSynthPro::reset() noexcept {
    std::memset(_cylPhases,  0, sizeof(_cylPhases));
    std::memset(_pulseAmps,  0, sizeof(_pulseAmps));
    std::memset(_harmPhases, 0, sizeof(_harmPhases));

    _smoothRpm      = targetRpm.load(std::memory_order_relaxed);
    _smoothThrottle = 0.0f;
    _prevThrottle   = 0.0f;
    _turboBoost     = 0.0f;
    _turboPhase     = 0.0f;
    _volLpfState    = 0.0f;
    _backfireCounter = 0;
    _backfireLevel   = 0.0f;
    _noiseState      = 0xF1E2D3C4u;

    _cachedFormantFreq0 = 0.0f;
    _cachedFormantFreq1 = 0.0f;
    _cachedIntakeFreq   = 0.0f;

    _comb.reset();
    _formant0.reset();
    _formant1.reset();
    _intakeFilter.reset();
    _backfireFilter.reset();
}

// ── Audio render ──────────────────────────────────────────────────────────────

void EngineSynthPro::render(float* buffer, int32_t numFrames) noexcept {

    // ── Snapshot all atomics once per buffer (not per sample) ──────────────
    const float tgtRpm      = targetRpm.load(std::memory_order_relaxed);
    const float tgtThrottle = targetThrottle.load(std::memory_order_relaxed);
    const int   cyl         = std::min(cylinders.load(std::memory_order_relaxed),
                                       kProMaxCylinders);
    const float noiseLvl    = noiseLevel.load(std::memory_order_relaxed);

    const float tGain       = turboGain.load(std::memory_order_relaxed);
    const float tSpeedRatio = turboSpeedRatio.load(std::memory_order_relaxed);
    const int   tBlades     = turboBladeCount.load(std::memory_order_relaxed);

    const float ff0         = formantFreq0.load(std::memory_order_relaxed);
    const float ff1         = formantFreq1.load(std::memory_order_relaxed);
    const float fq0         = formantQ0.load(std::memory_order_relaxed);
    const float fq1         = formantQ1.load(std::memory_order_relaxed);
    const float fg0         = formantGain0.load(std::memory_order_relaxed);
    const float fg1         = formantGain1.load(std::memory_order_relaxed);
    const float combFb      = combFeedback.load(std::memory_order_relaxed);

    float hw[kProMaxHarmonics];
    for (int h = 0; h < kProMaxHarmonics; ++h) {
        hw[h] = harmonicWeights[h].load(std::memory_order_relaxed);
    }

    // ── Handle backfire trigger (set from outside thread) ─────────────────
    if (triggerBackfire.exchange(false, std::memory_order_relaxed)) {
        _backfireCounter = kBackfireLength;
        _backfireLevel   = 0.55f;
        _backfireFilter.setBandpass(110.0f, 0.75f, kProSampleRate);
    }

    // Update comb feedback (safe to do once per buffer)
    _comb.feedback = combFb;

    // Lazily update formant filter coefficients when freq changes > 2 Hz
    if (std::abs(ff0 - _cachedFormantFreq0) > 2.0f) {
        _formant0.setBandpass(ff0, fq0, kProSampleRate);
        _cachedFormantFreq0 = ff0;
    }
    if (std::abs(ff1 - _cachedFormantFreq1) > 2.0f) {
        _formant1.setBandpass(ff1, fq1, kProSampleRate);
        _cachedFormantFreq1 = ff1;
    }

    const float nyquist = kProSampleRate * 0.5f;

    for (int32_t i = 0; i < numFrames; ++i) {

        // ── 1. Smooth RPM and throttle ────────────────────────────────────
        _smoothRpm      += (tgtRpm      - _smoothRpm)      * kRpmSmooth;
        _smoothThrottle += (tgtThrottle - _smoothThrottle) * kThrottleSmooth;

        // ── 2. Auto-detect throttle lift for backfire ─────────────────────
        if (_prevThrottle > 0.65f &&
            _smoothThrottle < 0.15f &&
            _smoothRpm > 2500.0f &&
            _backfireCounter == 0)
        {
            _backfireCounter = kBackfireLength / 2;
            _backfireLevel   = 0.35f + std::abs(nextNoise()) * 0.15f;
            _backfireFilter.setBandpass(
                90.0f + std::abs(nextNoise()) * 50.0f,
                0.65f, kProSampleRate);
        }
        _prevThrottle = _smoothThrottle;

        // ── 3. Fundamental firing frequency ───────────────────────────────
        // f₀ = RPM/60 × (cyl/2)  [4-stroke: fires every 2 revolutions]
        const float f0 = (_smoothRpm / 60.0f) *
                         (static_cast<float>(cyl) * 0.5f);
        const float dPhase = (f0 > 0.5f)
                           ? (kProTwoPi * f0 / kProSampleRate)
                           : 0.0f;

        // ── 4. Cylinder firing pulse model ────────────────────────────────
        float pulseSample = 0.0f;
        if (cyl > 0 && f0 > 1.0f) {
            for (int c = 0; c < cyl; ++c) {
                _cylPhases[c] += dPhase;
                if (_cylPhases[c] >= kProTwoPi) {
                    _cylPhases[c] -= kProTwoPi;
                    // Firing event: set pulse amplitude
                    // Modulate by throttle so idle is gentler
                    _pulseAmps[c] = 0.6f + _smoothThrottle * 0.4f;
                }
                // Render pulse: decaying oscillation at 2×f₀ (combustion knock freq)
                // clamp so high RPM doesn't exceed Nyquist
                const float pulseFreq = std::min(f0 * 2.5f, nyquist * 0.9f);
                const float pulsePhase = _cylPhases[c] * (pulseFreq / f0);
                pulseSample += _pulseAmps[c] * std::sin(pulsePhase);
                _pulseAmps[c] *= kPulseDecay;
            }
            pulseSample /= static_cast<float>(cyl);
        }

        // ── 5. Additive harmonic layer (tonal character) ──────────────────
        float harmSample    = 0.0f;
        float totalHarmW    = 0.0f;
        const float normRpm = _smoothRpm / 8000.0f;

        for (int h = 0; h < kProMaxHarmonics; ++h) {
            if (hw[h] < 0.001f) continue;
            const float freq = f0 * static_cast<float>(h + 1);
            if (freq >= nyquist || freq < 0.5f) continue;

            _harmPhases[h] += kProTwoPi * freq / kProSampleRate;
            if (_harmPhases[h] >= kProTwoPi) _harmPhases[h] -= kProTwoPi;

            // Upper harmonics bloom progressively with RPM
            const float emphasis = 1.0f + normRpm * 0.6f * static_cast<float>(h);
            const float amp      = hw[h] * emphasis;
            harmSample  += amp * std::sin(_harmPhases[h]);
            totalHarmW  += amp;
        }
        if (totalHarmW > 0.001f) harmSample /= totalHarmW;

        // ── 6. Mix: pulses give the "chug", harmonics give tonal colour ───
        float rawSample = pulseSample * 0.55f + harmSample * 0.45f;

        // ── 7. Soft saturation — increases with RPM (exhaust harmonics) ───
        {
            const float sat = normRpm * normRpm * 0.38f;
            rawSample = rawSample - sat * rawSample * rawSample * rawSample;
        }

        // ── 8. Comb filter (exhaust pipe standing-wave resonance) ─────────
        // Delay = Fs / (2·f₀) → resonance peak at f₀ and harmonics
        if (f0 > 1.0f) {
            const float combDelay = std::min(kProSampleRate / (2.0f * f0),
                                             static_cast<float>(kCombDelayMax - 2));
            rawSample = _comb.process(rawSample, combDelay);
        }

        // ── 9. Formant filter bank ────────────────────────────────────────
        // Add resonant colouring without replacing the dry signal
        const float formantContrib = _formant0.process(rawSample) * fg0
                                   + _formant1.process(rawSample) * fg1;
        rawSample = (rawSample + formantContrib) / (1.0f + fg0 + fg1);

        // ── 10. Intake noise (throttle-body induction roar) ───────────────
        if (noiseLvl > 0.001f) {
            const float intakeFreq = 280.0f + _smoothThrottle * 220.0f;
            if (std::abs(intakeFreq - _cachedIntakeFreq) > 5.0f) {
                _intakeFilter.setBandpass(intakeFreq, 1.4f, kProSampleRate);
                _cachedIntakeFreq = intakeFreq;
            }
            rawSample += _intakeFilter.process(nextNoise()) * noiseLvl;
        }

        // ── 11. Turbo whine ───────────────────────────────────────────────
        if (tGain > 0.001f) {
            // Boost builds with RPM × throttle (lags due to kBoostSmooth)
            const float boostTarget = (normRpm) * _smoothThrottle;
            _turboBoost += (boostTarget - _turboBoost) * kBoostSmooth;

            // Compressor whine frequency
            float tFreq = (_smoothRpm / 60.0f)
                        * tSpeedRatio
                        * static_cast<float>(tBlades);
            tFreq = std::min(tFreq, nyquist * 0.9f);

            _turboPhase += kProTwoPi * tFreq / kProSampleRate;
            if (_turboPhase >= kProTwoPi) _turboPhase -= kProTwoPi;

            // Whine = fundamental + 2nd sub-harmonic for richness
            const float whine = std::sin(_turboPhase)
                              + 0.30f * std::sin(_turboPhase * 2.0f)
                              + 0.10f * std::sin(_turboPhase * 3.0f);

            rawSample += whine * tGain * _turboBoost * 0.45f;
        }

        // ── 12. Backfire / pop ────────────────────────────────────────────
        if (_backfireCounter > 0) {
            const float t    = static_cast<float>(_backfireCounter)
                             / static_cast<float>(kBackfireLength);
            const float pop  = _backfireFilter.process(nextNoise())
                             * _backfireLevel * t * t;
            rawSample += pop;
            --_backfireCounter;
        }

        // ── 13. Volume envelope with anti-click LPF ───────────────────────
        const float targetVol = (_smoothRpm < 10.0f)
                              ? 0.0f
                              : 0.28f + _smoothThrottle * 0.72f;
        _volLpfState = targetVol + _volLpfCoeff * (_volLpfState - targetVol);
        rawSample *= _volLpfState;

        // ── 14. Hard-clip safety net ──────────────────────────────────────
        buffer[i] = std::fmax(-1.0f, std::fmin(1.0f, rawSample));
    }
}

// ── Noise PRNG (Galois LFSR) ──────────────────────────────────────────────────

float EngineSynthPro::nextNoise() noexcept {
    _noiseState ^= _noiseState << 13;
    _noiseState ^= _noiseState >> 17;
    _noiseState ^= _noiseState << 5;
    // Map uint32 → [-1, +1]
    return static_cast<float>(static_cast<int32_t>(_noiseState))
         / static_cast<float>(0x7FFFFFFFu);
}

} // namespace enginex
