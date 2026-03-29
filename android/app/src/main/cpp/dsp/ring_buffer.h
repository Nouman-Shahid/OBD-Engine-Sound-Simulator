#pragma once

#include <cstring>
#include <algorithm>

namespace enginex {

/**
 * Fixed-capacity ring buffer with linear interpolation read-back.
 *
 * Template parameter CAPACITY must be a power-of-two for best performance
 * (wrap uses bitwise AND instead of modulo).
 *
 * Used as the delay line in CombFilter and AllpassFilter.
 *
 * Entirely stack-allocatable — no heap, no locks, real-time safe.
 */
template<int CAPACITY>
struct RingBuffer {
    static_assert(CAPACITY > 1, "RingBuffer CAPACITY must be > 1");

    float buf[CAPACITY]{};
    int   writeIdx{0};

    void write(float x) noexcept {
        buf[writeIdx] = x;
        writeIdx = (writeIdx + 1) % CAPACITY;
    }

    /**
     * Read back [delaySamples] samples ago with linear interpolation.
     * delaySamples must be in (0, CAPACITY-1].
     */
    float readInterp(float delaySamples) const noexcept {
        const float capped = std::min(delaySamples, static_cast<float>(CAPACITY - 2));
        const int   d0     = static_cast<int>(capped);
        const float frac   = capped - static_cast<float>(d0);

        // Integer read positions (going back in time)
        const int i0 = (writeIdx - 1 - d0 + CAPACITY * 2) % CAPACITY;
        const int i1 = (i0 - 1 + CAPACITY) % CAPACITY;

        return buf[i0] * (1.0f - frac) + buf[i1] * frac;
    }

    /** Read an exact integer delay (no interpolation). */
    float readExact(int delaySamples) const noexcept {
        const int idx = (writeIdx - 1 - delaySamples + CAPACITY * 2) % CAPACITY;
        return buf[idx];
    }

    void reset() noexcept {
        std::memset(buf, 0, sizeof(buf));
        writeIdx = 0;
    }
};

} // namespace enginex
