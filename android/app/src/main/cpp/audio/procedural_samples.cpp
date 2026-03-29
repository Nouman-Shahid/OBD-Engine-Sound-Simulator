#include "procedural_samples.h"
#include <cmath>

namespace enginex {

// ── Static definitions ────────────────────────────────────────────────────────

float ProceduralSamples::intakeBuf  [ProceduralSamples::kIntakeLen];
float ProceduralSamples::exhaustBuf [ProceduralSamples::kExhaustLen];
float ProceduralSamples::backfireBuf[ProceduralSamples::kBackfireLen];
float ProceduralSamples::turboBuf   [ProceduralSamples::kTurboLen];

AudioSample ProceduralSamples::intake;
AudioSample ProceduralSamples::exhaust;
AudioSample ProceduralSamples::backfire;
AudioSample ProceduralSamples::turbo;

bool ProceduralSamples::_generated = false;

// ── Generation-time helpers ───────────────────────────────────────────────────

static constexpr float kTwoPi = 6.28318530717958647f;

/** Galois LFSR PRNG used only during generate() — not audio-thread safe. */
static uint32_t g_genNoise = 0xABCD1234u;
static float genNoise() noexcept {
    g_genNoise ^= g_genNoise << 13;
    g_genNoise ^= g_genNoise >> 17;
    g_genNoise ^= g_genNoise << 5;
    return static_cast<float>(static_cast<int32_t>(g_genNoise))
         / static_cast<float>(0x7FFFFFFFu);
}

/**
 * Fill buf[0..len) with bandpass-filtered white noise.
 *
 *  fc      centre frequency (Hz)
 *  bw      -3 dB bandwidth  (Hz)
 *  fs      sample rate      (Hz)
 *  gain    output amplitude scalar
 *
 * Uses a 2-pole resonant IIR (direct form I):
 *   y[n] = g*(x[n] - x[n-2]) + 2r*cos(w0)*y[n-1] - r²*y[n-2]
 * where r = 1 - π*bw/fs  (pole radius, ~Q = fc/bw)
 */
static void fillBandpassNoise(float* buf, int len,
                              float fc, float bw, float fs,
                              float gain) noexcept
{
    const float r  = 1.0f - (kTwoPi * 0.5f * bw / fs);
    const float w0 = kTwoPi * fc / fs;
    // Normalise gain so peak output ≈ gain
    const float g  = (1.0f - r) * gain * 2.0f;

    const float a1 =  2.0f * r * std::cos(w0);
    const float a2 = -(r * r);

    float xm1 = 0.0f, xm2 = 0.0f;
    float ym1 = 0.0f, ym2 = 0.0f;

    // Warm up — run 512 samples through filter to reach steady state
    for (int i = 0; i < 512; ++i) {
        const float x = genNoise();
        const float y = g * (x - xm2) - a2 * ym2 + a1 * ym1;
        xm2 = xm1; xm1 = x;
        ym2 = ym1; ym1 = y;
    }

    // Fill output buffer
    for (int i = 0; i < len; ++i) {
        const float x = genNoise();
        const float y = g * (x - xm2) - a2 * ym2 + a1 * ym1;
        xm2 = xm1; xm1 = x;
        ym2 = ym1; ym1 = y;
        buf[i] = y;
    }
}

// ── generate() ───────────────────────────────────────────────────────────────

void ProceduralSamples::generate(float sampleRate) noexcept {
    if (_generated) return;
    _generated = true;

    //
    // Intake — induction roar: centred at 600 Hz, BW = 500 Hz
    //   Models the air rushing past the throttle plate and velocity stacks.
    //   Warm low-mid texture, throttle-dependent amplitude in the render loop.
    //
    fillBandpassNoise(intakeBuf, kIntakeLen,
                      600.0f, 500.0f, sampleRate, 0.70f);

    //
    // Exhaust — exhaust texture: centred at 160 Hz, BW = 240 Hz
    //   Deep, low-frequency rumble that adds grit under comb/formant output.
    //   Amplitude modulated by engine load in the render loop.
    //
    fillBandpassNoise(exhaustBuf, kExhaustLen,
                      160.0f, 240.0f, sampleRate, 0.65f);

    //
    // Backfire — transient pop: centred at 500 Hz, BW = 800 Hz
    //   Broadband pop for mechanical click and sharp onset.
    //   Combined with the existing LFSR backfire burst in the render loop.
    //
    fillBandpassNoise(backfireBuf, kBackfireLen,
                      500.0f, 800.0f, sampleRate, 0.75f);

    //
    // Turbo — spool texture: centred at 7000 Hz, BW = 5000 Hz
    //   High-frequency hiss that layers over the turbo whine oscillator.
    //   Gives realistic "rush" quality to forced induction engines.
    //
    fillBandpassNoise(turboBuf, kTurboLen,
                      7000.0f, 5000.0f, sampleRate, 0.55f);

    // ── Initialise AudioSample descriptors ───────────────────────────────────
    //
    // Loop region starts at frame 256 to skip the filter warm-up transient
    // that is present at the very start of each buffer despite the pre-roll.

    intake  = { intakeBuf,   kIntakeLen,
                256.0f, static_cast<float>(kIntakeLen)  };

    exhaust = { exhaustBuf,  kExhaustLen,
                256.0f, static_cast<float>(kExhaustLen) };

    // Backfire is one-shot — no loop region
    backfire = { backfireBuf, kBackfireLen, 0.0f, 0.0f };

    turbo    = { turboBuf,  kTurboLen,
                 256.0f, static_cast<float>(kTurboLen)  };
}

} // namespace enginex
