import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/features/engine/data/models/engine_state.dart';
import 'package:enginex/features/engine/data/models/vehicle_profile.dart';
import 'package:enginex/features/engine/domain/engine_notifier.dart';
import 'package:enginex/services/platform_channel_service.dart';

// ── Core engine state ─────────────────────────────────────────────────────

final engineProvider =
    StateNotifierProvider<EngineNotifier, EngineState>((ref) {
  final channel = ref.watch(platformChannelServiceProvider);
  return EngineNotifier(channel);
});

// ── Derived / scoped providers (select prevents unnecessary rebuilds) ──────

final rpmProvider = Provider<double>(
  (ref) => ref.watch(engineProvider.select((s) => s.rpm)),
);

final throttleProvider = Provider<double>(
  (ref) => ref.watch(engineProvider.select((s) => s.throttle)),
);

final gearProvider = Provider<int>(
  (ref) => ref.watch(engineProvider.select((s) => s.gear)),
);

final isEngineRunningProvider = Provider<bool>(
  (ref) => ref.watch(engineProvider.select((s) => s.isRunning)),
);

final isRevLimitingProvider = Provider<bool>(
  (ref) => ref.watch(engineProvider.select((s) => s.isRevLimiting)),
);

final activeLayerProvider = Provider<AudioLayer>(
  (ref) => ref.watch(engineProvider.select((s) => s.activeLayer)),
);

// ── Vehicle catalogue ─────────────────────────────────────────────────────

final vehicleCatalogueProvider = Provider<List<VehicleProfile>>(
  (_) => VehicleCatalogue.all,
);

final selectedVehicleProvider = Provider<VehicleProfile>((ref) {
  final id = ref.watch(engineProvider.select((s) => s.vehicleId));
  return VehicleCatalogue.findById(id);
});
