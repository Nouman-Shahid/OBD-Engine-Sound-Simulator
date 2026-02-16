/// Defines all platform channel names and method/event keys.
/// Single source of truth shared by the Dart layer — mirrors the
/// constants in EngineChannelHandler.kt.
library;

class ChannelConstants {
  ChannelConstants._();

  // ── Channel names ──────────────────────────────────────────────────────────
  static const String methodChannel = 'com.enginex/engine_method';
  static const String eventChannel  = 'com.enginex/engine_event';

  // ── Method names (Flutter → Native) ───────────────────────────────────────
  static const String methodStartEngine  = 'startEngine';
  static const String methodStopEngine   = 'stopEngine';
  static const String methodSetThrottle  = 'setThrottle';
  static const String methodSetGear      = 'setGear';
  static const String methodSetVehicle   = 'setVehicle';
  static const String methodIsEngineRunning = 'isEngineRunning';

  // ── Argument keys ─────────────────────────────────────────────────────────
  static const String argThrottle  = 'throttle';
  static const String argGear      = 'gear';
  static const String argVehicleId = 'vehicleId';

  // ── Event keys (Native → Flutter) ─────────────────────────────────────────
  static const String eventRpm          = 'rpm';
  static const String eventEngineState  = 'engineState';
  static const String eventGear         = 'gear';
  static const String eventThrottle     = 'throttle';
}
