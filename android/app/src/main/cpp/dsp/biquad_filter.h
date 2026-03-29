#pragma once

#include <cmath>

namespace enginex {

/**
 * Biquad filter — Direct Form II Transposed.
 *
 * Supports:
 *   setLowpass(freq, Q, Fs)
 *   setHighpass(freq, Q, Fs)
 *   setBandpass(freq, Q, Fs)   — constant-skirt gain, peak = Q
 *   setPeakEQ(freq, dBgain, Q, Fs)
 *
 * Entirely inline and allocation-free — safe for audio-thread use.
 * Reset state with reset() when switching filter type or on stream restart.
 */
struct BiquadFilter {
    // Coefficients
    float b0{1.0f}, b1{0.0f}, b2{0.0f};
    float          a1{0.0f}, a2{0.0f};
    // State (Direct Form II transposed)
    float z1{0.0f}, z2{0.0f};

    // ── Coefficient setters ──────────────────────────────────────────────────

    void setLowpass(float freq, float Q, float sampleRate) noexcept {
        const float w0    = kTwoPi * freq / sampleRate;
        const float cosw  = std::cos(w0);
        const float alpha = std::sin(w0) / (2.0f * Q);
        const float a0inv = 1.0f / (1.0f + alpha);
        b0 = ((1.0f - cosw) * 0.5f) * a0inv;
        b1 = ( 1.0f - cosw)          * a0inv;
        b2 = b0;
        a1 = (-2.0f * cosw)          * a0inv;
        a2 = ( 1.0f - alpha)         * a0inv;
    }

    void setHighpass(float freq, float Q, float sampleRate) noexcept {
        const float w0    = kTwoPi * freq / sampleRate;
        const float cosw  = std::cos(w0);
        const float alpha = std::sin(w0) / (2.0f * Q);
        const float a0inv = 1.0f / (1.0f + alpha);
        b0 = ((1.0f + cosw) * 0.5f) * a0inv;
        b1 = -(1.0f + cosw)          * a0inv;
        b2 = b0;
        a1 = (-2.0f * cosw)          * a0inv;
        a2 = ( 1.0f - alpha)         * a0inv;
    }

    /**
     * Bandpass (constant skirt gain, peak gain = Q).
     * Useful for formant / resonance modelling.
     */
    void setBandpass(float freq, float Q, float sampleRate) noexcept {
        const float w0    = kTwoPi * freq / sampleRate;
        const float sinw  = std::sin(w0);
        const float cosw  = std::cos(w0);
        const float alpha = sinw / (2.0f * Q);
        const float a0inv = 1.0f / (1.0f + alpha);
        b0 = (sinw * 0.5f)  * a0inv;
        b1 = 0.0f;
        b2 = -(sinw * 0.5f) * a0inv;
        a1 = (-2.0f * cosw) * a0inv;
        a2 = ( 1.0f - alpha) * a0inv;
    }

    /**
     * Peaking EQ — boosts/cuts by dBgain around freq with bandwidth Q.
     * Used for harmonic shaping.
     */
    void setPeakEQ(float freq, float dBgain, float Q, float sampleRate) noexcept {
        const float A     = std::pow(10.0f, dBgain / 40.0f);
        const float w0    = kTwoPi * freq / sampleRate;
        const float alpha = std::sin(w0) / (2.0f * Q);
        const float cosw  = std::cos(w0);
        const float a0inv = 1.0f / (1.0f + alpha / A);
        b0 = ( 1.0f + alpha * A) * a0inv;
        b1 = (-2.0f * cosw)      * a0inv;
        b2 = ( 1.0f - alpha * A) * a0inv;
        a1 = b1;
        a2 = ( 1.0f - alpha / A) * a0inv;
    }

    /**
     * Low-shelf: boost/cut below freq.
     */
    void setLowshelf(float freq, float dBgain, float sampleRate) noexcept {
        const float A     = std::pow(10.0f, dBgain / 40.0f);
        const float w0    = kTwoPi * freq / sampleRate;
        const float cosw  = std::cos(w0);
        const float sqrtA = std::sqrt(A);
        const float alpha = std::sin(w0) * 0.5f * std::sqrt((A + 1.0f/A) * (1.0f/1.0f - 1.0f) + 2.0f);
        const float a0inv = 1.0f / (  (A+1.0f) + (A-1.0f)*cosw + 2.0f*sqrtA*alpha );
        b0 = A *((A+1.0f) - (A-1.0f)*cosw + 2.0f*sqrtA*alpha) * a0inv;
        b1 = 2.0f*A*((A-1.0f) - (A+1.0f)*cosw)                * a0inv;
        b2 = A *((A+1.0f) - (A-1.0f)*cosw - 2.0f*sqrtA*alpha) * a0inv;
        a1 = -2.0f*((A-1.0f) + (A+1.0f)*cosw)                 * a0inv;
        a2 = ((A+1.0f) + (A-1.0f)*cosw - 2.0f*sqrtA*alpha)    * a0inv;
    }

    // ── Audio processing ─────────────────────────────────────────────────────

    inline float process(float x) noexcept {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept {
        z1 = z2 = 0.0f;
    }

private:
    static constexpr float kTwoPi = 6.28318530717958647f;
};

} // namespace enginex
