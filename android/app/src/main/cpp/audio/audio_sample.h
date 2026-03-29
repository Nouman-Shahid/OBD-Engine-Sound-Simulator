#pragma once
#include <cstdint>

namespace enginex {

/**
 * Descriptor for a read-only PCM sample buffer.
 *
 * Data is owned externally (e.g. a static array in ProceduralSamples).
 * This struct holds only a pointer + length + loop region — no ownership.
 */
struct AudioSample {
    const float* data      {nullptr};
    int32_t      length    {0};
    float        loopStart {0.0f};  ///< First frame of seamless loop region
    float        loopEnd   {0.0f};  ///< One-past-last frame (0 = use length)
};

} // namespace enginex
