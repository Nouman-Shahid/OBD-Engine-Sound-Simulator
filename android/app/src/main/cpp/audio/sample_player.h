#pragma once
#include "audio_sample.h"
#include <cmath>

namespace enginex {

/**
 * Single-voice looping sample player.
 *
 * Features:
 *   - Variable playback speed (linear interpolation — no extra allocation)
 *   - Seamless loop between [loopStart, loopEnd) in the AudioSample
 *   - Gain control applied to each output sample
 *
 * Thread safety: NOT thread-safe. All access must be from the audio thread.
 *
 * Usage (audio thread):
 *   player.load(&myAudioSample);
 *   player.speed = rpmRatio;
 *   player.gain  = 0.3f;
 *   float out = player.tick();   // called per sample inside render()
 */
struct SamplePlayer {

    float speed {1.0f};  ///< Playback rate multiplier (1.0 = original pitch)
    float gain  {1.0f};  ///< Output scale [0, 1+]

    /** Attach a sample buffer. Resets position to loopStart. */
    void load(const AudioSample* sample) noexcept {
        _sample = sample;
        _pos    = (sample && sample->loopStart > 0.0f)
                ? sample->loopStart
                : 0.0f;
    }

    /** Reset read position to loop start without changing sample or speed. */
    void reset() noexcept {
        if (_sample) {
            _pos = (_sample->loopStart > 0.0f) ? _sample->loopStart : 0.0f;
        }
    }

    /** Return one interpolated, gain-scaled sample and advance position. */
    inline float tick() noexcept {
        if (!_sample || _sample->length == 0) return 0.0f;

        const float loopEnd =
            (_sample->loopEnd > 0.0f)
            ? _sample->loopEnd
            : static_cast<float>(_sample->length);
        const float loopStart = _sample->loopStart;
        const float loopLen   = loopEnd - loopStart;

        // Wrap within loop region (guard against loopLen <= 0)
        if (loopLen > 0.0f) {
            while (_pos >= loopEnd)   _pos -= loopLen;
            while (_pos <  loopStart) _pos  = loopStart;
        }

        // Linear interpolation between adjacent samples
        const int32_t i0   = static_cast<int32_t>(_pos);
        const float   frac = _pos - static_cast<float>(i0);
        const int32_t i1   = i0 + 1;

        const float s0 = _sample->data[i0];
        const float s1 = (i1 < _sample->length) ? _sample->data[i1] : s0;

        _pos += speed;

        return (s0 + frac * (s1 - s0)) * gain;
    }

private:
    const AudioSample* _sample {nullptr};
    float              _pos    {0.0f};
};

} // namespace enginex
