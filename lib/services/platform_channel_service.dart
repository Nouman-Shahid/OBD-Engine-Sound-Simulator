import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/core/constants/channel_constants.dart';
import 'package:enginex/features/engine/data/models/engine_state.dart';
import 'package:enginex/features/engine/data/models/obd_state.dart';

/// Low-level wrapper around both Flutter ↔ Android platform channels.
///
/// Engine channels:
///   [MethodChannel]  Flutter → Native (commands)
///   [EventChannel]   Native → Flutter (state stream ~60 Hz)
///
/// OBD channels:
///   [MethodChannel]  Flutter → Native (scan, connect, disconnect)
///   [EventChannel]   Native → Flutter (connection events + data)
class PlatformChannelService {
  PlatformChannelService()
      : _method    = const MethodChannel(ChannelConstants.methodChannel),
        _event     = const EventChannel(ChannelConstants.eventChannel),
        _obdMethod = const MethodChannel(ChannelConstants.obdMethodChannel),
        _obdEvent  = const EventChannel(ChannelConstants.obdEventChannel);

  final MethodChannel _method;
  final EventChannel  _event;
  final MethodChannel _obdMethod;
  final EventChannel  _obdEvent;

  StreamSubscription<dynamic>? _eventSub;
  StreamSubscription<dynamic>? _obdSub;

  final StreamController<EngineState> _stateController =
      StreamController<EngineState>.broadcast();
  final StreamController<OBDState> _obdController =
      StreamController<OBDState>.broadcast();

  // ── Public streams ────────────────────────────────────────────────────────

  Stream<EngineState> get stateStream => _stateController.stream;
  Stream<OBDState>    get obdStream   => _obdController.stream;

  // ── Lifecycle ─────────────────────────────────────────────────────────────

  void startListening() {
    _eventSub?.cancel();
    _eventSub = _event
        .receiveBroadcastStream()
        .listen(_onNativeEvent, onError: _onNativeError);

    _obdSub?.cancel();
    _obdSub = _obdEvent
        .receiveBroadcastStream()
        .listen(_onObdEvent, onError: _onNativeError);
  }

  void dispose() {
    _eventSub?.cancel();
    _obdSub?.cancel();
    _stateController.close();
    _obdController.close();
  }

  // ── Engine commands ───────────────────────────────────────────────────────

  Future<bool> startEngine() async {
    final result = await _method.invokeMethod<bool>(ChannelConstants.methodStartEngine);
    return result ?? false;
  }

  Future<void> stopEngine() =>
      _method.invokeMethod<void>(ChannelConstants.methodStopEngine);

  Future<void> setThrottle(double throttle) =>
      _method.invokeMethod<void>(ChannelConstants.methodSetThrottle, {
        ChannelConstants.argThrottle: throttle.clamp(0.0, 1.0),
      });

  Future<void> setGear(int gear) =>
      _method.invokeMethod<void>(ChannelConstants.methodSetGear, {
        ChannelConstants.argGear: gear.clamp(0, 6),
      });

  Future<void> setVehicle(String vehicleId) =>
      _method.invokeMethod<void>(ChannelConstants.methodSetVehicle, {
        ChannelConstants.argVehicleId: vehicleId,
      });

  Future<bool> isEngineRunning() async {
    final result = await _method.invokeMethod<bool>(ChannelConstants.methodIsEngineRunning);
    return result ?? false;
  }

  Future<void> triggerBackfire() =>
      _method.invokeMethod<void>(ChannelConstants.methodTriggerBackfire);

  // ── OBD commands ──────────────────────────────────────────────────────────

  /// Returns a list of paired Bluetooth devices as [BTDeviceInfo].
  Future<List<BTDeviceInfo>> obdScanDevices() async {
    final raw = await _obdMethod.invokeMethod<List>(ChannelConstants.obdScanDevices);
    if (raw == null) return [];
    return raw.map((e) {
      final m = Map<String, dynamic>.from(e as Map);
      return BTDeviceInfo.fromMap(m);
    }).toList();
  }

  /// Connect to ELM327 device at [address].
  Future<void> obdConnect(String address) =>
      _obdMethod.invokeMethod<void>(ChannelConstants.obdConnect, {
        ChannelConstants.argAddress: address,
      });

  Future<void> obdDisconnect() =>
      _obdMethod.invokeMethod<void>(ChannelConstants.obdDisconnect);

  Future<bool> obdIsConnected() async {
    final result = await _obdMethod.invokeMethod<bool>(ChannelConstants.obdIsConnected);
    return result ?? false;
  }

  // ── Private event handlers ────────────────────────────────────────────────

  void _onNativeEvent(dynamic raw) {
    if (raw is! Map) return;
    final map = Map<String, dynamic>.from(raw as Map);

    final rpm        = (map[ChannelConstants.eventRpm]      as num?)?.toDouble() ?? 0.0;
    final throttle   = (map[ChannelConstants.eventThrottle] as num?)?.toDouble() ?? 0.0;
    final gear       = (map[ChannelConstants.eventGear]     as num?)?.toInt()    ?? 0;
    final stateStr   = map[ChannelConstants.eventEngineState] as String? ?? 'stopped';
    final isRunning  = stateStr != 'stopped';
    final isLimiting = stateStr == 'rev_limiting';
    final layerStr   = map[ChannelConstants.eventActiveLayer] as String? ?? 'idle';

    _stateController.add(EngineState(
      rpm:           rpm,
      throttle:      throttle,
      gear:          gear,
      isRunning:     isRunning,
      isRevLimiting: isLimiting,
      activeLayer:   AudioLayerParsing.fromString(layerStr),
    ));
  }

  void _onObdEvent(dynamic raw) {
    if (raw is! Map) return;
    final map   = Map<String, dynamic>.from(raw as Map);
    final event = map[ChannelConstants.obdEvent] as String? ?? '';

    switch (event) {
      case ChannelConstants.obdEventConnected:
        _obdController.add(OBDState(
          isConnected:   true,
          deviceAddress: map[ChannelConstants.obdAddress] as String? ?? '',
        ));
      case ChannelConstants.obdEventDisconnected:
        _obdController.add(OBDState.disconnected);
      case ChannelConstants.obdEventData:
        final rpm      = (map[ChannelConstants.obdRpm]     as num?)?.toDouble() ?? 0.0;
        final throttle = (map[ChannelConstants.obdThrottle] as num?)?.toDouble() ?? 0.0;
        _obdController.add(OBDState(
          isConnected: true,
          rpm:         rpm,
          throttle:    throttle,
        ));
    }
  }

  void _onNativeError(Object error) {
    // ignore: avoid_print
    print('[EngineX] Channel error: $error');
  }
}

// ── Riverpod provider ──────────────────────────────────────────────────────

final platformChannelServiceProvider = Provider<PlatformChannelService>((ref) {
  final service = PlatformChannelService()..startListening();
  ref.onDispose(service.dispose);
  return service;
});
