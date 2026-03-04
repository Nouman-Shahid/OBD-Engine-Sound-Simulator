# OBD Engine Sound Simulator

A cross-platform Flutter application that simulates realistic engine sounds using OBD-II style vehicle profiles, RPM modelling, and native Android audio playback via platform channels.

## Repository Structure

| Directory | Role |
|---|---|
| `lib/core/` | Theme, constants, and shared utilities |
| `lib/features/engine/` | Engine feature — data, domain, and presentation layers |
| `lib/services/` | Platform channel bridge to native Android audio |
| `src/` | Android native Java for OBD sound playback |
| `assets/audio/` | Engine sound samples at idle, mid, and high RPM |

## Features

- **Realistic RPM Simulation** — per-vehicle RPM curve with idle baseline, redline ceiling, and throttle-driven acceleration
- **Gear Shift Logic** — manual up/down shifting with RPM drop on upshift and spike on downshift
- **Native Audio Playback** — Android MethodChannel drives real engine sound samples modulated by RPM pitch
- **Vehicle Profiles** — built-in presets for sedan, SUV, sports car, and truck with distinct engine characteristics
- **Live Engine Dashboard** — animated RPM gauge, throttle slider, gear indicator, and engine status panel
- **Dark UI** — purpose-built dark theme optimised for in-vehicle use

## Tech Stack

- Flutter 3 (Dart) — cross-platform UI
- `flutter_riverpod` — reactive state management
- `freezed` + `json_serializable` — immutable models with code generation
- `google_fonts` — custom typography
- Android MethodChannel (Java) — native engine sound modulation

## Getting Started

```bash
flutter pub get
dart run build_runner build --delete-conflicting-outputs
flutter run
```

## Platform Channel

The `PlatformChannelService` calls the `enginex/audio` MethodChannel. The Android host (`src/Main.java`) receives RPM values and adjusts playback pitch and sample selection in real time.

