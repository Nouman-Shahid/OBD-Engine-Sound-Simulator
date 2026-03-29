/// Platform channel names and keys — single source of truth for Dart.
/// Mirrors the constants in [EngineChannelHandler.kt].
library;

class ChannelConstants {
  ChannelConstants._();

  // ── Engine channel names ───────────────────────────────────────────────────
  static const String methodChannel = 'com.enginex/engine_method';
  static const String eventChannel  = 'com.enginex/engine_event';

  // ── OBD channel names ──────────────────────────────────────────────────────
  static const String obdMethodChannel = 'com.enginex/obd_method';
  static const String obdEventChannel  = 'com.enginex/obd_event';

  // ── Engine method names (Flutter → Native) ─────────────────────────────────
  static const String methodStartEngine    = 'startEngine';
  static const String methodStopEngine     = 'stopEngine';
  static const String methodSetThrottle    = 'setThrottle';
  static const String methodSetGear        = 'setGear';
  static const String methodSetVehicle     = 'setVehicle';
  static const String methodIsEngineRunning = 'isEngineRunning';
  static const String methodTriggerBackfire = 'triggerBackfire';

  // ── OBD method names (Flutter → Native) ───────────────────────────────────
  static const String obdScanDevices  = 'scanDevices';
  static const String obdConnect      = 'connect';
  static const String obdDisconnect   = 'disconnect';
  static const String obdIsConnected  = 'isConnected';

  // ── Argument keys ──────────────────────────────────────────────────────────
  static const String argThrottle   = 'throttle';
  static const String argGear       = 'gear';
  static const String argVehicleId  = 'vehicleId';
  static const String argAddress    = 'address';

  // ── Engine event keys (Native → Flutter) ──────────────────────────────────
  static const String eventRpm         = 'rpm';
  static const String eventThrottle    = 'throttle';
  static const String eventGear        = 'gear';
  static const String eventEngineState = 'engineState';
  static const String eventActiveLayer = 'activeLayer';

  // ── OBD event keys (Native → Flutter) ─────────────────────────────────────
  static const String obdEvent     = 'event';
  static const String obdEventConnected    = 'connected';
  static const String obdEventDisconnected = 'disconnected';
  static const String obdEventData         = 'data';
  static const String obdRpm       = 'rpm';
  static const String obdThrottle  = 'throttle';
  static const String obdAddress   = 'address';
}
