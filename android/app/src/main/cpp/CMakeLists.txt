cmake_minimum_required(VERSION 3.22.1)

project(enginex_audio VERSION 2.0.0 LANGUAGES CXX)

# ── C++ standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ── Oboe (fetched from Prefab AAR via build.gradle) ──────────────────────────
find_package(oboe REQUIRED CONFIG)

# ── Source files ─────────────────────────────────────────────────────────────
add_library(
    enginex_audio
    SHARED
        oboe_engine.cpp
        jni_bridge.cpp
        synth/engine_synth_pro.cpp
        audio/procedural_samples.cpp
        # Legacy synth kept for reference — not linked into the binary
        # synth/engine_synth.cpp
)

# ── Include paths ─────────────────────────────────────────────────────────────
target_include_directories(enginex_audio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/synth
    ${CMAKE_CURRENT_SOURCE_DIR}/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
)

# ── Linked libraries ──────────────────────────────────────────────────────────
target_link_libraries(enginex_audio
    oboe::oboe
    android
    log
    atomic
)

# ── Compile flags ─────────────────────────────────────────────────────────────
target_compile_options(enginex_audio PRIVATE
    -O3
    -ffast-math
    -fno-exceptions     # keep binary lean; EngineSynthPro uses no exceptions
    -fno-rtti
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# ── Link-time optimisation (release only) ────────────────────────────────────
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET enginex_audio PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
