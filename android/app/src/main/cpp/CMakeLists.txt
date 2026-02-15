cmake_minimum_required(VERSION 3.22.1)

project(enginex_audio VERSION 1.0.0 LANGUAGES CXX)

# ── C++ standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ── Oboe (fetched from prefab AAR via build.gradle) ─────────────────────────
find_package(oboe REQUIRED CONFIG)

# ── Source files ────────────────────────────────────────────────────────────
add_library(
    enginex_audio
    SHARED
        oboe_engine.cpp
        jni_bridge.cpp
        synth/engine_synth.cpp
)

# ── Include paths ────────────────────────────────────────────────────────────
target_include_directories(enginex_audio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/synth
)

# ── Linked libraries ─────────────────────────────────────────────────────────
target_link_libraries(enginex_audio
    oboe::oboe
    android
    log
    atomic
)

# ── Compile flags ────────────────────────────────────────────────────────────
target_compile_options(enginex_audio PRIVATE
    -O3
    -ffast-math
    -fno-exceptions   # keep binary lean; we don't use C++ exceptions
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# Enable link-time optimisation for release builds.
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET enginex_audio PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
