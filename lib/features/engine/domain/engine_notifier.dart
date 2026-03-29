import 'dart:async';

import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/features/engine/data/models/engine_state.dart';
import 'package:enginex/features/engine/data/models/obd_state.dart';
import 'package:enginex/features/engine/data/models/vehicle_profile.dart';
import 'package:enginex/services/platform_channel_service.dart';

/// Drives all engine interaction from the UI layer.
///
/// Subscribes to the native EventChannel stream and exposes a clean
/// [EngineState] to widgets, throttling UI rebuilds via Riverpod select.
class EngineNotifier extends StateNotifier<EngineState> {
  EngineNotifier(this._channel) : super(const EngineState()) {
    _sub = _channel.stateStream.listen(_onState);
  }

  final PlatformChannelService _channel;
  late final StreamSubscription<EngineState> _sub;

  // ── Engine lifecycle ───────────────────────────────────────────────────────

  Future<void> startEngine() async {
    final ok = await _channel.startEngine();
    if (ok) state = state.copyWith(isRunning: true);
  }

  Future<void> stopEngine() async {
    await _channel.stopEngine();
    state = state.copyWith(isRunning: false, rpm: 0.0, throttle: 0.0);
  }

  // ── Real-time controls ─────────────────────────────────────────────────────

  Future<void> setThrottle(double value) async {
    state = state.copyWith(throttle: value);   // optimistic update
    await _channel.setThrottle(value);
  }

  Future<void> shiftUp() async {
    final next = (state.gear + 1).clamp(0, 6);
    if (next == state.gear) return;
    state = state.copyWith(gear: next);
    await _channel.setGear(next);
  }

  Future<void> shiftDown() async {
    final prev = (state.gear - 1).clamp(0, 6);
    if (prev == state.gear) return;
    state = state.copyWith(gear: prev);
    await _channel.setGear(prev);
  }

  Future<void> setGear(int gear) async {
    final g = gear.clamp(0, 6);
    state = state.copyWith(gear: g);
    await _channel.setGear(g);
  }

  Future<void> selectVehicle(VehicleProfile profile) async {
    await _channel.setVehicle(profile.id);
    state = state.copyWith(vehicleId: profile.id);
  }

  Future<void> triggerBackfire() => _channel.triggerBackfire();

  // ── Private ────────────────────────────────────────────────────────────────

  void _onState(EngineState incoming) {
    // Preserve vehicleId — not sent over the native event stream
    state = incoming.copyWith(vehicleId: state.vehicleId);
  }

  @override
  void dispose() {
    _sub.cancel();
    super.dispose();
  }
}

/// Manages OBD-II connection state and forwards device data to [EngineNotifier].
class OBDNotifier extends StateNotifier<OBDState> {
  OBDNotifier(this._channel) : super(OBDState.disconnected) {
    _sub = _channel.obdStream.listen(_onObdEvent);
  }

  final PlatformChannelService _channel;
  late final StreamSubscription<OBDState> _sub;

  // ── Public API ─────────────────────────────────────────────────────────────

  Future<List<BTDeviceInfo>> scanDevices() => _channel.obdScanDevices();

  Future<void> connect(String address) => _channel.obdConnect(address);

  Future<void> disconnect() {
    state = OBDState.disconnected;
    return _channel.obdDisconnect();
  }

  // ── Private ────────────────────────────────────────────────────────────────

  void _onObdEvent(OBDState event) {
    state = event;
  }

  @override
  void dispose() {
    _sub.cancel();
    super.dispose();
  }
}
