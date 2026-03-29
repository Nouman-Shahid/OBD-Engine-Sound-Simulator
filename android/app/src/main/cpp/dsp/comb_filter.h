#pragma once

#include "ring_buffer.h"

namespace enginex {

/**
 * Variable-delay comb filter (Schroeder-type, positive feedback).
 *
 * Transfer function: H(z) = 1 / (1 − g · z^{-D})
 *
 * Used to model exhaust pipe resonance. The delay D tracks the
 * instantaneous fundamental period:
 *
 *   D = Fs / (2 · f₀)    [half-wavelength at firing frequency]
 *
 * This gives a resonance peak exactly at f₀ and all its harmonics,
 * recreating the narrow-band boost that a real exhaust pipe produces.
 *
 * Feedback g controls the resonance decay:
 *   g = 0   → flat (comb off)
 *   g = 0.3 → moderate resonance (realistic exhaust)
 *   g > 0.7 → strong metallic ringing (not recommended)
 *
 * Template parameter CAPACITY: size of the internal ring buffer.
 * Must be large enough to hold the longest delay (lowest RPM).
 * At 48 kHz and 300 RPM minimum for a 4-cyl:
 *   f₀_min = 300/60 × 2 = 10 Hz  →  D_max = 48000/(2×10) = 2400
 * Use CAPACITY = 4096 for safety.
 */
template<int CAPACITY = 4096>
struct CombFilter {
    RingBuffer<CAPACITY> delay;
    float feedback{0.30f};

    /** Process one sample with variable delay D (fractional samples). */
    float process(float x, float delaySamples) noexcept {
        const float delayed = delay.readInterp(delaySamples);
        const float out     = x + feedback * delayed;
        delay.write(out);
        return out;
    }

    void reset() noexcept {
        delay.reset();
    }
};

/**
 * Allpass filter (Schroeder) used in reverb tails and diffusion.
 * Not used in the main engine path but available for future use.
 */
template<int CAPACITY = 1024>
struct AllpassFilter {
    RingBuffer<CAPACITY> delay;
    float gain{0.5f};
    int   delayLen{500};

    float process(float x) noexcept {
        const float buf  = delay.readExact(delayLen);
        const float v    = x + gain * buf;
        delay.write(v);
        return buf - gain * v;
    }

    void reset() noexcept { delay.reset(); }
};

} // namespace enginex
