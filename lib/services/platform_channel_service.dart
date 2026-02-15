import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/core/constants/channel_constants.dart';
import 'package:enginex/features/engine/data/models/engine_state.dart';

/// Low-level wrapper around the Flutter ↔ Android platform channels.
///
/// - [MethodChannel]  Flutter → Native (commands)
/// - [EventChannel]   Native → Flutter (state stream at ~60 Hz)
class PlatformChannelService {
  PlatformChannelService()
      : _method = const MethodChannel(ChannelConstants.methodChannel),
        _event  = const EventChannel(ChannelConstants.eventChannel);

  final MethodChannel _method;
  final EventChannel  _event;

  StreamSubscription<dynamic>? _eventSub;
  final StreamController<EngineState> _stateController =
      StreamController<EngineState>.broadcast();

  /// Live stream of [EngineState] pushed from the native layer.
  Stream<EngineState> get stateStream => _stateController.stream;

  // ── Lifecycle ──────────────────────────────────────────────────────────────

  void startListening() {
    _eventSub?.cancel();
    _eventSub = _event
        .receiveBroadcastStream()
        .listen(_onNativeEvent, onError: _onNativeError);
  }

  void dispose() {
    _eventSub?.cancel();
    _stateController.close();
  }

  // ── Commands ───────────────────────────────────────────────────────────────

  Future<bool> startEngine() async {
    final result = await _method.invokeMethod<bool>(
      ChannelConstants.methodStartEngine,
    );
    return result ?? false;
  }

  Future<void> stopEngine() =>
      _method.invokeMethod<void>(ChannelConstants.methodStopEngine);

  /// [throttle] must be in [0.0, 1.0].
  Future<void> setThrottle(double throttle) =>
      _method.invokeMethod<void>(ChannelConstants.methodSetThrottle, {
        ChannelConstants.argThrottle: throttle.clamp(0.0, 1.0),
      });

  /// [gear] must be in [0, 6] where 0 = neutral.
  Future<void> setGear(int gear) =>
      _method.invokeMethod<void>(ChannelConstants.methodSetGear, {
        ChannelConstants.argGear: gear.clamp(0, 6),
      });

  Future<void> setVehicle(String vehicleId) =>
      _method.invokeMethod<void>(ChannelConstants.methodSetVehicle, {
        ChannelConstants.argVehicleId: vehicleId,
      });

  Future<bool> isEngineRunning() async {
    final result = await _method.invokeMethod<bool>(
      ChannelConstants.methodIsEngineRunning,
    );
    return result ?? false;
  }

  // ── Private ────────────────────────────────────────────────────────────────

  void _onNativeEvent(dynamic raw) {
    if (raw is! Map) return;
    final map = Map<String, dynamic>.from(raw as Map);

    final rpm         = (map[ChannelConstants.eventRpm] as num?)?.toDouble() ?? 0.0;
    final throttle    = (map[ChannelConstants.eventThrottle] as num?)?.toDouble() ?? 0.0;
    final gear        = (map[ChannelConstants.eventGear] as num?)?.toInt() ?? 0;
    final stateStr    = map[ChannelConstants.eventEngineState] as String? ?? 'stopped';
    final isRunning   = stateStr != 'stopped';
    final isLimiting  = stateStr == 'rev_limiting';
    final layerStr    = map['activeLayer'] as String? ?? 'idle';

    _stateController.add(EngineState(
      rpm:           rpm,
      throttle:      throttle,
      gear:          gear,
      isRunning:     isRunning,
      isRevLimiting: isLimiting,
      activeLayer:   AudioLayerParsing.fromString(layerStr),
    ));
  }

  void _onNativeError(Object error) {
    // In production, send to crash reporting service.
    // ignore: avoid_print
    print('[EngineX] EventChannel error: $error');
  }
}

// ── Riverpod provider ──────────────────────────────────────────────────────

final platformChannelServiceProvider = Provider<PlatformChannelService>((ref) {
  final service = PlatformChannelService()..startListening();
  ref.onDispose(service.dispose);
  return service;
});
