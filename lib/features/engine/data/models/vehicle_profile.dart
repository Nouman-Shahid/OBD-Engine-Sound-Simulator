import 'package:freezed_annotation/freezed_annotation.dart';

part 'vehicle_profile.freezed.dart';
part 'vehicle_profile.g.dart';

/// Describes the physical characteristics of a vehicle's powertrain.
/// These values are passed to the native engine simulator so it can
/// produce RPM behaviour that is unique to each vehicle type.
@freezed
class VehicleProfile with _$VehicleProfile {
  const factory VehicleProfile({
    required String id,
    required String name,
    required String description,

    /// Number of cylinders (2, 4, 6, 8, …).
    required int cylinders,

    /// Maximum engine RPM before rev-limiter cuts in.
    required double maxRpm,

    /// Idle RPM when throttle = 0.
    required double idleRpm,

    /// RPM above which rev-limiter engages.
    required double revLimiterRpm,

    /// How quickly RPM rises toward target (0–1, lower = slower).
    required double inertiaFactor,

    /// Per-gear final-drive ratio multiplied by gear ratio.
    required List<double> gearRatios,

    /// Harmonic weights that colour the synth sound [h1..h8].
    required List<double> harmonicWeights,

    /// Noise blend level (0–1) — gives the engine its "breath".
    required double noiseLevel,

    /// Emoji / icon identifier shown in the vehicle picker.
    required String emoji,
  }) = _VehicleProfile;

  factory VehicleProfile.fromJson(Map<String, dynamic> json) =>
      _$VehicleProfileFromJson(json);
}

// ── Built-in vehicle catalogue ─────────────────────────────────────────────

abstract class VehicleCatalogue {
  static const List<VehicleProfile> all = [
    _sport4,
    _muscle8,
    _inline6,
    _singleCyl,
    _v6Turbo,
  ];

  static VehicleProfile findById(String id) =>
      all.firstWhere((v) => v.id == id, orElse: () => _sport4);

  // ── 2.0 L Sport 4-cylinder ───────────────────────────────────────────────
  static const _sport4 = VehicleProfile(
    id: 'sport_4cyl',
    name: 'Sport 4-Cyl',
    description: '2.0 L turbocharged 4-cylinder — punchy mid-range.',
    cylinders: 4,
    maxRpm: 7200.0,
    idleRpm: 850.0,
    revLimiterRpm: 7000.0,
    inertiaFactor: 0.08,
    gearRatios: [0.0, 3.36, 1.99, 1.33, 1.00, 0.82, 0.64],
    harmonicWeights: [1.0, 0.6, 0.35, 0.18, 0.10, 0.06, 0.04, 0.02],
    noiseLevel: 0.08,
    emoji: '🚗',
  );

  // ── 5.7 L Muscle V8 ──────────────────────────────────────────────────────
  static const _muscle8 = VehicleProfile(
    id: 'muscle_v8',
    name: 'Muscle V8',
    description: '5.7 L naturally-aspirated V8 — deep growl, torque-heavy.',
    cylinders: 8,
    maxRpm: 6500.0,
    idleRpm: 750.0,
    revLimiterRpm: 6200.0,
    inertiaFactor: 0.05,
    gearRatios: [0.0, 2.97, 1.78, 1.26, 1.00, 0.84, 0.68],
    harmonicWeights: [1.0, 0.8, 0.5, 0.30, 0.16, 0.10, 0.06, 0.04],
    noiseLevel: 0.06,
    emoji: '🏎️',
  );

  // ── 3.0 L Inline-6 ───────────────────────────────────────────────────────
  static const _inline6 = VehicleProfile(
    id: 'inline_6',
    name: 'Inline-6',
    description: '3.0 L silky smooth inline-6 — balanced harmonics.',
    cylinders: 6,
    maxRpm: 7000.0,
    idleRpm: 800.0,
    revLimiterRpm: 6800.0,
    inertiaFactor: 0.07,
    gearRatios: [0.0, 3.83, 2.20, 1.52, 1.22, 1.00, 0.81],
    harmonicWeights: [0.9, 0.7, 0.55, 0.35, 0.20, 0.12, 0.07, 0.04],
    noiseLevel: 0.05,
    emoji: '🚙',
  );

  // ── Single-cylinder 450 cc ───────────────────────────────────────────────
  static const _singleCyl = VehicleProfile(
    id: 'single_cyl',
    name: 'Single Cyl Bike',
    description: '450 cc single-cylinder — raw thumper character.',
    cylinders: 1,
    maxRpm: 9500.0,
    idleRpm: 1200.0,
    revLimiterRpm: 9200.0,
    inertiaFactor: 0.12,
    gearRatios: [0.0, 2.85, 1.80, 1.32, 1.04, 0.86, 0.70],
    harmonicWeights: [1.0, 0.4, 0.25, 0.15, 0.08, 0.05, 0.03, 0.02],
    noiseLevel: 0.12,
    emoji: '🏍️',
  );

  // ── 3.5 L V6 Turbo ───────────────────────────────────────────────────────
  static const _v6Turbo = VehicleProfile(
    id: 'v6_turbo',
    name: 'V6 Turbo',
    description: '3.5 L bi-turbo V6 — aggressive spool, flat power curve.',
    cylinders: 6,
    maxRpm: 8000.0,
    idleRpm: 900.0,
    revLimiterRpm: 7800.0,
    inertiaFactor: 0.09,
    gearRatios: [0.0, 3.55, 2.10, 1.45, 1.10, 0.88, 0.71],
    harmonicWeights: [0.85, 0.75, 0.60, 0.40, 0.25, 0.14, 0.08, 0.05],
    noiseLevel: 0.10,
    emoji: '⚡',
  );
}
