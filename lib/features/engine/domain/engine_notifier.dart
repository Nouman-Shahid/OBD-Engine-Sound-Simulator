import 'dart:async';

import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/features/engine/data/models/engine_state.dart';
import 'package:enginex/features/engine/data/models/vehicle_profile.dart';
import 'package:enginex/services/platform_channel_service.dart';

/// Drives all engine interaction from the UI layer.
/// Subscribes to the native EventChannel stream and exposes a clean
/// [EngineState] to widgets while throttling UI re-builds.
class EngineNotifier extends StateNotifier<EngineState> {
  EngineNotifier(this._channel) : super(const EngineState()) {
    _sub = _channel.stateStream.listen(_onState);
  }

  final PlatformChannelService _channel;
  late final StreamSubscription<EngineState> _sub;

  // ── Public API ─────────────────────────────────────────────────────────────

  Future<void> startEngine() async {
    await _channel.startEngine();
    state = state.copyWith(isRunning: true);
  }

  Future<void> stopEngine() async {
    await _channel.stopEngine();
    state = state.copyWith(isRunning: false, rpm: 0.0, throttle: 0.0);
  }

  Future<void> setThrottle(double value) async {
    // Optimistic local update for immediate UI feedback.
    state = state.copyWith(throttle: value);
    await _channel.setThrottle(value);
  }

  Future<void> shiftUp() async {
    final nextGear = (state.gear + 1).clamp(0, 6);
    if (nextGear == state.gear) return;
    state = state.copyWith(gear: nextGear);
    await _channel.setGear(nextGear);
  }

  Future<void> shiftDown() async {
    final prevGear = (state.gear - 1).clamp(0, 6);
    if (prevGear == state.gear) return;
    state = state.copyWith(gear: prevGear);
    await _channel.setGear(prevGear);
  }

  Future<void> setGear(int gear) async {
    final clamped = gear.clamp(0, 6);
    state = state.copyWith(gear: clamped);
    await _channel.setGear(clamped);
  }

  Future<void> selectVehicle(VehicleProfile profile) async {
    await _channel.setVehicle(profile.id);
    state = state.copyWith(vehicleId: profile.id);
  }

  // ── Private ────────────────────────────────────────────────────────────────

  void _onState(EngineState incoming) {
    // Preserve vehicleId which is not sent in the native event stream.
    state = incoming.copyWith(vehicleId: state.vehicleId);
  }

  @override
  void dispose() {
    _sub.cancel();
    super.dispose();
  }
}
