import 'package:freezed_annotation/freezed_annotation.dart';

part 'vehicle_profile.freezed.dart';
part 'vehicle_profile.g.dart';

/// Describes the physical and acoustic characteristics of a vehicle powertrain.
///
/// These values drive both the RPM physics simulation in Kotlin (inertia,
/// gear ratios, RPM limits) and the C++ DSP engine (harmonics, formant
/// filters, comb filter, turbo layer).
@freezed
class VehicleProfile with _$VehicleProfile {
  const factory VehicleProfile({
    required String id,
    required String name,
    required String description,

    /// Number of cylinders: determines firing frequency f₀ = RPM/60 × (cyl/2).
    required int cylinders,

    /// Maximum display RPM on the gauge.
    required double maxRpm,

    /// Idle RPM (throttle = 0).
    required double idleRpm,

    /// Rev-limiter cut-in RPM.
    required double revLimiterRpm,

    /// RPM ramp inertia [0, 1]: lower = heavier flywheel / slower response.
    required double inertiaFactor,

    /// Final-drive × gear ratios [neutral, 1st..6th].
    required List<double> gearRatios,

    // ── DSP parameters ────────────────────────────────────────────────────────

    /// Per-harmonic amplitude weights [h0..h11].
    required List<double> harmonicWeights,

    /// Intake breath noise level [0, 1].
    required double noiseLevel,

    /// Exhaust comb-filter feedback — resonance strength [0, 0.5].
    required double combFeedback,

    /// Low formant centre frequency (Hz) — body resonance.
    required double formantFreq0,

    /// High formant centre frequency (Hz) — intake/exhaust character.
    required double formantFreq1,

    /// Low formant bandwidth (Q factor).
    required double formantQ0,

    /// High formant bandwidth (Q factor).
    required double formantQ1,

    /// Low formant mix level [0, 1].
    required double formantGain0,

    /// High formant mix level [0, 1].
    required double formantGain1,

    // ── Turbo ─────────────────────────────────────────────────────────────────

    /// Turbo/supercharger whine amplitude [0 = no forced induction].
    @Default(0.0) double turboGain,

    /// Compressor speed / engine speed ratio (typically 12–16 for turbo).
    @Default(14.0) double turboSpeedRatio,

    /// Number of compressor blades (typically 9–13).
    @Default(11) int turboBladeCount,

    /// Emoji / icon for vehicle picker UI.
    required String emoji,
  }) = _VehicleProfile;

  factory VehicleProfile.fromJson(Map<String, dynamic> json) =>
      _$VehicleProfileFromJson(json);
}

// ── Built-in vehicle catalogue ─────────────────────────────────────────────

abstract class VehicleCatalogue {
  static const List<VehicleProfile> all = [
    _sport4,
    _muscleV8,
    _inline6,
    _singleCyl,
    _v6Turbo,
    _supercardV10,
    _flat6,
    _turbo4,
  ];

  static VehicleProfile findById(String id) =>
      all.firstWhere((v) => v.id == id, orElse: () => _sport4);

  // ── 2.0 L Sport 4-Cylinder ───────────────────────────────────────────────
  static const _sport4 = VehicleProfile(
    id:            'sport_4cyl',
    name:          'Sport 4-Cyl',
    description:   '2.0 L turbocharged 4-cylinder — punchy mid-range.',
    cylinders:     4,
    maxRpm:        7200.0,
    idleRpm:       850.0,
    revLimiterRpm: 7000.0,
    inertiaFactor: 0.08,
    gearRatios:    [0.0, 3.36, 1.99, 1.33, 1.00, 0.82, 0.64],
    harmonicWeights: [1.0,0.60,0.35,0.18,0.10,0.06,0.04,0.02,0.01,0.005,0.002,0.001],
    noiseLevel:    0.08,
    combFeedback:  0.28,
    formantFreq0:  310.0,
    formantFreq1:  1150.0,
    formantQ0:     2.5,
    formantQ1:     3.0,
    formantGain0:  0.35,
    formantGain1:  0.25,
    emoji:         '🚗',
  );

  // ── 5.7 L Muscle V8 ──────────────────────────────────────────────────────
  static const _muscleV8 = VehicleProfile(
    id:            'muscle_v8',
    name:          'Muscle V8',
    description:   '5.7 L naturally-aspirated V8 — deep growl, torque-heavy.',
    cylinders:     8,
    maxRpm:        6500.0,
    idleRpm:       750.0,
    revLimiterRpm: 6200.0,
    inertiaFactor: 0.05,
    gearRatios:    [0.0, 2.97, 1.78, 1.26, 1.00, 0.84, 0.68],
    harmonicWeights: [1.0,0.80,0.50,0.30,0.16,0.10,0.06,0.04,0.02,0.01,0.005,0.002],
    noiseLevel:    0.06,
    combFeedback:  0.32,
    formantFreq0:  180.0,
    formantFreq1:  750.0,
    formantQ0:     2.0,
    formantQ1:     2.8,
    formantGain0:  0.42,
    formantGain1:  0.30,
    emoji:         '🏎️',
  );

  // ── 3.0 L Inline-6 ───────────────────────────────────────────────────────
  static const _inline6 = VehicleProfile(
    id:            'inline_6',
    name:          'Inline-6',
    description:   '3.0 L silky smooth inline-6 — balanced harmonics.',
    cylinders:     6,
    maxRpm:        7000.0,
    idleRpm:       800.0,
    revLimiterRpm: 6800.0,
    inertiaFactor: 0.07,
    gearRatios:    [0.0, 3.83, 2.20, 1.52, 1.22, 1.00, 0.81],
    harmonicWeights: [0.90,0.70,0.55,0.35,0.20,0.12,0.07,0.04,0.02,0.01,0.005,0.002],
    noiseLevel:    0.05,
    combFeedback:  0.25,
    formantFreq0:  260.0,
    formantFreq1:  1050.0,
    formantQ0:     3.0,
    formantQ1:     3.5,
    formantGain0:  0.30,
    formantGain1:  0.22,
    emoji:         '🚙',
  );

  // ── Single-Cylinder 450 cc ───────────────────────────────────────────────
  static const _singleCyl = VehicleProfile(
    id:            'single_cyl',
    name:          'Single Cyl Bike',
    description:   '450 cc single-cylinder — raw thumper character.',
    cylinders:     1,
    maxRpm:        9500.0,
    idleRpm:       1200.0,
    revLimiterRpm: 9200.0,
    inertiaFactor: 0.12,
    gearRatios:    [0.0, 2.85, 1.80, 1.32, 1.04, 0.86, 0.70],
    harmonicWeights: [1.0,0.40,0.25,0.15,0.08,0.05,0.03,0.02,0.01,0.006,0.003,0.001],
    noiseLevel:    0.12,
    combFeedback:  0.22,
    formantFreq0:  420.0,
    formantFreq1:  1400.0,
    formantQ0:     1.8,
    formantQ1:     2.2,
    formantGain0:  0.28,
    formantGain1:  0.20,
    emoji:         '🏍️',
  );

  // ── 3.5 L V6 Bi-Turbo ────────────────────────────────────────────────────
  static const _v6Turbo = VehicleProfile(
    id:             'v6_turbo',
    name:           'V6 Bi-Turbo',
    description:    '3.5 L bi-turbo V6 — aggressive spool, flat power curve.',
    cylinders:      6,
    maxRpm:         8000.0,
    idleRpm:        900.0,
    revLimiterRpm:  7800.0,
    inertiaFactor:  0.09,
    gearRatios:     [0.0, 3.55, 2.10, 1.45, 1.10, 0.88, 0.71],
    harmonicWeights: [0.85,0.75,0.60,0.40,0.25,0.14,0.08,0.05,0.03,0.015,0.008,0.003],
    noiseLevel:     0.10,
    combFeedback:   0.30,
    formantFreq0:   290.0,
    formantFreq1:   1100.0,
    formantQ0:      2.8,
    formantQ1:      3.2,
    formantGain0:   0.33,
    formantGain1:   0.27,
    turboGain:      0.14,
    turboSpeedRatio: 15.0,
    turboBladeCount: 11,
    emoji:          '⚡',
  );

  // ── 5.0 L Supercar V10 ───────────────────────────────────────────────────
  static const _supercardV10 = VehicleProfile(
    id:            'supercar_v10',
    name:          'Supercar V10',
    description:   '5.0 L naturally-aspirated V10 — screaming 9k RPM.',
    cylinders:     10,
    maxRpm:        9200.0,
    idleRpm:       800.0,
    revLimiterRpm: 8800.0,
    inertiaFactor: 0.06,
    gearRatios:    [0.0, 3.09, 2.19, 1.72, 1.36, 1.09, 0.87],
    harmonicWeights: [1.0,0.65,0.40,0.22,0.12,0.07,0.04,0.025,0.015,0.008,0.004,0.002],
    noiseLevel:    0.07,
    combFeedback:  0.34,
    formantFreq0:  240.0,
    formantFreq1:  980.0,
    formantQ0:     3.2,
    formantQ1:     4.0,
    formantGain0:  0.38,
    formantGain1:  0.30,
    emoji:         '🔥',
  );

  // ── 4.0 L Flat-6 (Boxer) ─────────────────────────────────────────────────
  static const _flat6 = VehicleProfile(
    id:            'flat_6',
    name:          'Flat-6 Boxer',
    description:   '4.0 L air-cooled flat-6 — the iconic Porsche-style rumble.',
    cylinders:     6,
    maxRpm:        8500.0,
    idleRpm:       900.0,
    revLimiterRpm: 8200.0,
    inertiaFactor: 0.065,
    gearRatios:    [0.0, 3.82, 2.26, 1.64, 1.28, 1.03, 0.84],
    harmonicWeights: [0.95,0.55,0.65,0.45,0.28,0.17,0.10,0.06,0.03,0.015,0.008,0.003],
    noiseLevel:    0.09,
    combFeedback:  0.26,
    formantFreq0:  350.0,
    formantFreq1:  1300.0,
    formantQ0:     2.2,
    formantQ1:     2.8,
    formantGain0:  0.32,
    formantGain1:  0.26,
    emoji:         '🏁',
  );

  // ── 2.0 L 4-Cyl Turbo (hot hatch) ────────────────────────────────────────
  static const _turbo4 = VehicleProfile(
    id:             'turbo_4cyl',
    name:           '4-Cyl Turbo',
    description:    '2.0 L 4-cyl turbo hot hatch — spooling induction roar.',
    cylinders:      4,
    maxRpm:         7000.0,
    idleRpm:        900.0,
    revLimiterRpm:  6800.0,
    inertiaFactor:  0.07,
    gearRatios:     [0.0, 3.46, 1.95, 1.36, 1.03, 0.85, 0.70],
    harmonicWeights: [0.95,0.65,0.40,0.22,0.13,0.08,0.05,0.03,0.015,0.008,0.004,0.002],
    noiseLevel:     0.09,
    combFeedback:   0.27,
    formantFreq0:   340.0,
    formantFreq1:   1200.0,
    formantQ0:      2.6,
    formantQ1:      3.1,
    formantGain0:   0.34,
    formantGain1:   0.26,
    turboGain:      0.11,
    turboSpeedRatio: 14.5,
    turboBladeCount: 10,
    emoji:          '💨',
  );
}
