#pragma once
#include "audio_sample.h"
#include <cstdint>

namespace enginex {

/**
 * Procedurally generated sample banks for the hybrid audio engine.
 *
 * All PCM data lives in static BSS arrays — zero heap allocation.
 * Call generate() once before opening the Oboe stream.
 *
 * ┌──────────┬───────────────┬──────────────────────────────────────────┐
 * │ Bank     │ Length        │ Content                                  │
 * ├──────────┼───────────────┼──────────────────────────────────────────┤
 * │ intake   │ 16384 frames  │ Induction roar — 300–900 Hz bandpass     │
 * │ exhaust  │ 16384 frames  │ Exhaust texture — 80–400 Hz bandpass     │
 * │ backfire │  8192 frames  │ Pop transient  — 100–1200 Hz bandpass    │
 * │ turbo    │  8192 frames  │ Spool texture  — 3–12 kHz bandpass       │
 * └──────────┴───────────────┴──────────────────────────────────────────┘
 *
 * Intake, exhaust, and turbo are configured with loop regions so they
 * can be played back seamlessly at variable speed. Backfire is one-shot.
 */
struct ProceduralSamples {

    static constexpr int kIntakeLen   = 16384;  // ~341 ms @ 48 kHz
    static constexpr int kExhaustLen  = 16384;
    static constexpr int kBackfireLen =  8192;  // ~170 ms
    static constexpr int kTurboLen    =  8192;

    // ── Static PCM storage (BSS — no runtime alloc) ───────────────────────
    static float intakeBuf  [kIntakeLen];
    static float exhaustBuf [kExhaustLen];
    static float backfireBuf[kBackfireLen];
    static float turboBuf   [kTurboLen];

    // ── AudioSample descriptors ───────────────────────────────────────────
    static AudioSample intake;
    static AudioSample exhaust;
    static AudioSample backfire;
    static AudioSample turbo;

    /**
     * Generate all four sample banks at the given sample rate.
     * Must be called from the main thread before the audio stream starts.
     * Calling again has no effect (idempotent via _generated guard).
     */
    static void generate(float sampleRate) noexcept;

private:
    static bool _generated;
};

} // namespace enginex
