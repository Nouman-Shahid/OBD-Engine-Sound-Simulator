import 'package:freezed_annotation/freezed_annotation.dart';

part 'engine_state.freezed.dart';

/// Immutable snapshot of the engine simulator at a given moment.
/// Updated at ~60 Hz from native → Flutter via the EventChannel.
@freezed
class EngineState with _$EngineState {
  const factory EngineState({
    /// Current smoothed RPM (after inertia model).
    @Default(0.0) double rpm,

    /// Throttle position [0.0 – 1.0].
    @Default(0.0) double throttle,

    /// Current gear (0 = neutral, 1–6 = drive gears).
    @Default(0) int gear,

    /// Whether the engine is running.
    @Default(false) bool isRunning,

    /// Rev-limiter is cutting fuel (>= revLimiterRpm).
    @Default(false) bool isRevLimiting,

    /// Active audio layer descriptor for debug/UI overlay.
    @Default(AudioLayer.idle) AudioLayer activeLayer,

    /// ID of the currently selected vehicle.
    @Default('sport_4cyl') String vehicleId,
  }) = _EngineState;

  const EngineState._();

  /// Normalised RPM in [0, 1] relative to maxRpm (8 000 for display).
  double get normalizedRpm => (rpm / 8000.0).clamp(0.0, 1.0);

  /// Returns the colour zone for the gauge needle.
  RpmZone get rpmZone {
    if (rpm < 2000) return RpmZone.idle;
    if (rpm < 4000) return RpmZone.low;
    if (rpm < 6000) return RpmZone.mid;
    if (rpm < 7000) return RpmZone.high;
    return RpmZone.danger;
  }
}

enum AudioLayer { idle, low, mid, high }

enum RpmZone { idle, low, mid, high, danger }

/// Extension so we can map native string values back to the enum.
extension AudioLayerParsing on AudioLayer {
  static AudioLayer fromString(String s) {
    switch (s) {
      case 'low':  return AudioLayer.low;
      case 'mid':  return AudioLayer.mid;
      case 'high': return AudioLayer.high;
      default:     return AudioLayer.idle;
    }
  }
}
