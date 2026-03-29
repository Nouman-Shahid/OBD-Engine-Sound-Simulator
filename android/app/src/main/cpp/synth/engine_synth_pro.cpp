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

    // Generate procedural sample banks (idempotent — safe to call multiple times)
    ProceduralSamples::generate(kProSampleRate);

    // Attach sample players to their respective banks
    _intakePlayer.load(&ProceduralSamples::intake);
    _exhaustPlayer.load(&ProceduralSamples::exhaust);
    _turboPlayer.load(&ProceduralSamples::turbo);
    _backfirePlayer.load(&ProceduralSamples::backfire);

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
    _outLpfState    = 0.0f;
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

    _intakePlayer.reset();
    _exhaustPlayer.reset();
    _turboPlayer.reset();
    _backfirePlayer.reset();
}

// ── Audio render ──────────────────────────────────────────────────────────────

void EngineSynthPro::render(float* buffer, int32_t numFrames) noexcept {

    // ── Snapshot all atomics once per buffer (not per sample) ──────────────
    const float tgtRpm      = targetRpm.load(std::memory_order_relaxed);
    const float tgtThrottle = targetThrottle.load(std::memory_order_relaxed);
    const int   cyl         = std::min(cylinders.load(std::memory_order_relaxed),
                                       kProMaxCylinders);
    const int   snapGear    = gear.load(std::memory_order_relaxed);
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

    const float iSmpGain    = intakeSampleGain.load(std::memory_order_relaxed);
    const float eSmpGain    = exhaustSampleGain.load(std::memory_order_relaxed);

    float hw[kProMaxHarmonics];
    for (int h = 0; h < kProMaxHarmonics; ++h) {
        hw[h] = harmonicWeights[h].load(std::memory_order_relaxed);
    }

    // ── Gear-dependent load factor ─────────────────────────────────────────
    // Gear 0 = neutral (free rev, low load). Gear 6 = high gear, full load.
    // loadFactor = throttle * [0.6 + 0.4 * (gear/6)]
    // Applied to comb feedback and saturation for realistic under-load sound.
    const float gearNorm = (snapGear > 0)
                         ? static_cast<float>(snapGear) / 6.0f
                         : 0.0f;

    // ── Throttle-dependent output LPF ──────────────────────────────────────
    // Cutoff ranges from 1500 Hz (idle = dark, muted) to 9000 Hz (WOT = bright).
    // Coefficient computed once per buffer — negligible error across 256 frames.
    const float lpfCutoff = 1500.0f + tgtThrottle * 7500.0f;
    const float lpfAlpha  = std::exp(-kProTwoPi * lpfCutoff / kProSampleRate);

    // ── Handle backfire trigger (set from outside thread) ──────────────────
    if (triggerBackfire.exchange(false, std::memory_order_relaxed)) {
        _backfireCounter = kBackfireLength;
        _backfireLevel   = 0.55f;
        _backfireFilter.setBandpass(110.0f, 0.75f, kProSampleRate);
        _backfirePlayer.reset();
    }

    // ── Lazily update formant filter coefficients ──────────────────────────
    if (std::abs(ff0 - _cachedFormantFreq0) > 2.0f) {
        _formant0.setBandpass(ff0, fq0, kProSampleRate);
        _cachedFormantFreq0 = ff0;
    }
    if (std::abs(ff1 - _cachedFormantFreq1) > 2.0f) {
        _formant1.setBandpass(ff1, fq1, kProSampleRate);
        _cachedFormantFreq1 = ff1;
    }

    // ── Sample player speeds ───────────────────────────────────────────────
    // Noise textures don't truly "pitch-shift" in a musically meaningful way,
    // but varying speed randomises the loop phase differently each rev and
    // prevents audible repetition artefacts. Range: 0.5–3.0×.
    _intakePlayer.speed  = std::max(0.5f, std::min(3.0f, tgtRpm / 2500.0f));
    _exhaustPlayer.speed = std::max(0.5f, std::min(3.0f, tgtRpm / 2000.0f));
    _turboPlayer.speed   = (tGain > 0.001f)
                         ? std::max(0.5f, std::min(3.0f, tgtRpm / 3000.0f))
                         : 0.0f;

    const float nyquist = kProSampleRate * 0.5f;

    for (int32_t i = 0; i < numFrames; ++i) {

        // ── 1. Smooth RPM and throttle ────────────────────────────────────
        _smoothRpm      += (tgtRpm      - _smoothRpm)      * kRpmSmooth;
        _smoothThrottle += (tgtThrottle - _smoothThrottle) * kThrottleSmooth;

        // ── 2. Engine load factor (gear + throttle) ───────────────────────
        // Higher gear at same throttle = more load on engine (physically correct)
        const float loadFactor = _smoothThrottle * (0.6f + gearNorm * 0.4f);

        // ── 3. Auto-detect throttle lift for backfire ─────────────────────
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
            _backfirePlayer.reset();
        }
        _prevThrottle = _smoothThrottle;

        // ── 4. Fundamental firing frequency ───────────────────────────────
        // f₀ = RPM/60 × (cyl/2)  [4-stroke: fires every 2 revolutions]
        const float f0 = (_smoothRpm / 60.0f) *
                         (static_cast<float>(cyl) * 0.5f);
        const float dPhase = (f0 > 0.5f)
                           ? (kProTwoPi * f0 / kProSampleRate)
                           : 0.0f;

        // ── 5. Cylinder firing pulse model (with phase jitter) ────────────
        float pulseSample = 0.0f;
        if (cyl > 0 && f0 > 1.0f) {
            for (int c = 0; c < cyl; ++c) {
                // Phase jitter: subtle stochastic timing variation per cylinder
                // Models manufacturing tolerances, combustion variation.
                _cylPhases[c] += dPhase + nextNoise() * kJitterScale;
                if (_cylPhases[c] >= kProTwoPi) {
                    _cylPhases[c] -= kProTwoPi;
                    // Firing event: pulse amplitude scales with throttle
                    _pulseAmps[c] = 0.6f + _smoothThrottle * 0.4f;
                }
                // Decaying oscillation at 2.5×f₀ (combustion knock freq)
                const float pulseFreq = std::min(f0 * 2.5f, nyquist * 0.9f);
                const float pulsePhase = _cylPhases[c] * (pulseFreq / f0);
                pulseSample += _pulseAmps[c] * std::sin(pulsePhase);
                _pulseAmps[c] *= kPulseDecay;
            }
            pulseSample /= static_cast<float>(cyl);
        }

        // ── 6. Additive harmonic layer (tonal character) ──────────────────
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

        // ── 7. Mix: pulses = chug, harmonics = tonal colour ───────────────
        float rawSample = pulseSample * 0.55f + harmSample * 0.45f;

        // ── 8. Soft saturation — load-modulated ───────────────────────────
        // Base saturation from RPM²; extra saturation from engine load.
        // Under heavy load (high gear + full throttle) sounds more compressed.
        {
            const float sat = (normRpm * normRpm + loadFactor * 0.18f) * 0.38f;
            rawSample = rawSample - sat * rawSample * rawSample * rawSample;
        }

        // ── 9. Comb filter (load-boosted exhaust resonance) ───────────────
        // Load increases feedback → more resonance in the exhaust pipe model
        if (f0 > 1.0f) {
            _comb.feedback = combFb + loadFactor * 0.10f;
            const float combDelay = std::min(kProSampleRate / (2.0f * f0),
                                             static_cast<float>(kCombDelayMax - 2));
            rawSample = _comb.process(rawSample, combDelay);
        } else {
            _comb.feedback = combFb;
        }

        // ── 10. Formant filter bank ───────────────────────────────────────
        const float formantContrib = _formant0.process(rawSample) * fg0
                                   + _formant1.process(rawSample) * fg1;
        rawSample = (rawSample + formantContrib) / (1.0f + fg0 + fg1);

        // ── 11. Exhaust texture sample layer ──────────────────────────────
        // Deep broadband grit that adds realism under load; fades at idle.
        if (eSmpGain > 0.001f) {
            const float exhaust = _exhaustPlayer.tick()
                                * eSmpGain
                                * (0.25f + loadFactor * 0.75f);
            rawSample += exhaust;
        }

        // ── 12. Intake noise (LFSR + sample blend) ────────────────────────
        if (noiseLvl > 0.001f) {
            // Filtered LFSR: rapid micro-variation
            const float intakeFreq = 280.0f + _smoothThrottle * 220.0f;
            if (std::abs(intakeFreq - _cachedIntakeFreq) > 5.0f) {
                _intakeFilter.setBandpass(intakeFreq, 1.4f, kProSampleRate);
                _cachedIntakeFreq = intakeFreq;
            }
            const float lfsrContrib = _intakeFilter.process(nextNoise())
                                    * noiseLvl * 0.50f;

            // Sample player: organic, non-repetitive induction texture
            const float smpContrib = _intakePlayer.tick()
                                   * iSmpGain
                                   * (0.30f + _smoothThrottle * 0.70f);

            rawSample += lfsrContrib + smpContrib;
        }

        // ── 13. Turbo whine + spool texture ──────────────────────────────
        if (tGain > 0.001f) {
            const float boostTarget = normRpm * _smoothThrottle;
            _turboBoost += (boostTarget - _turboBoost) * kBoostSmooth;

            float tFreq = (_smoothRpm / 60.0f)
                        * tSpeedRatio
                        * static_cast<float>(tBlades);
            tFreq = std::min(tFreq, nyquist * 0.9f);

            _turboPhase += kProTwoPi * tFreq / kProSampleRate;
            if (_turboPhase >= kProTwoPi) _turboPhase -= kProTwoPi;

            const float whine = std::sin(_turboPhase)
                              + 0.30f * std::sin(_turboPhase * 2.0f)
                              + 0.10f * std::sin(_turboPhase * 3.0f);

            // Broadband spool texture blended with tonal whine
            const float turboNoise = _turboPlayer.tick() * 0.35f;

            rawSample += (whine + turboNoise) * tGain * _turboBoost * 0.45f;
        }

        // ── 14. Backfire / pop ────────────────────────────────────────────
        if (_backfireCounter > 0) {
            const float t    = static_cast<float>(_backfireCounter)
                             / static_cast<float>(kBackfireLength);
            // LFSR pop
            const float lfsrPop  = _backfireFilter.process(nextNoise())
                                 * _backfireLevel * t * t;
            // Sample-based pop (lower-mid body thump)
            const float smpPop   = _backfirePlayer.tick()
                                 * _backfireLevel * t;
            rawSample += lfsrPop + smpPop * 0.4f;
            --_backfireCounter;
        }

        // ── 15. Output low-pass (throttle-dependent brightness) ───────────
        // One-pole IIR: dark at idle (fc≈1500 Hz), open at WOT (fc≈9000 Hz)
        _outLpfState = (1.0f - lpfAlpha) * rawSample + lpfAlpha * _outLpfState;
        rawSample = _outLpfState;

        // ── 16. Volume envelope with anti-click LPF ───────────────────────
        const float targetVol = (_smoothRpm < 10.0f)
                              ? 0.0f
                              : 0.28f + _smoothThrottle * 0.72f;
        _volLpfState = targetVol + _volLpfCoeff * (_volLpfState - targetVol);
        rawSample *= _volLpfState;

        // ── 17. Hard-clip safety net ──────────────────────────────────────
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
