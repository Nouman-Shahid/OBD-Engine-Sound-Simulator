import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/core/theme/app_theme.dart';
import 'package:enginex/features/engine/data/models/obd_state.dart';
import 'package:enginex/features/engine/domain/engine_providers.dart';

/// OBD-II Bluetooth device scanner and connection screen.
///
/// Shows paired Bluetooth devices. User taps a device to connect.
/// Once connected, OBD RPM + throttle feed into the engine simulation.
class OBDConnectScreen extends ConsumerStatefulWidget {
  const OBDConnectScreen({super.key});

  @override
  ConsumerState<OBDConnectScreen> createState() => _OBDConnectScreenState();
}

class _OBDConnectScreenState extends ConsumerState<OBDConnectScreen> {
  List<BTDeviceInfo> _devices = [];
  bool _scanning = false;
  String? _connectingAddress;

  @override
  void initState() {
    super.initState();
    _scan();
  }

  Future<void> _scan() async {
    setState(() { _scanning = true; _devices = []; });
    try {
      final devices = await ref.read(obdProvider.notifier).scanDevices();
      if (mounted) setState(() { _devices = devices; });
    } finally {
      if (mounted) setState(() { _scanning = false; });
    }
  }

  Future<void> _connect(String address) async {
    setState(() => _connectingAddress = address);
    try {
      await ref.read(obdProvider.notifier).connect(address);
      if (mounted) Navigator.of(context).pop();
    } finally {
      if (mounted) setState(() => _connectingAddress = null);
    }
  }

  @override
  Widget build(BuildContext context) {
    final obdState = ref.watch(obdProvider);

    return Scaffold(
      backgroundColor: AppTheme.background,
      appBar: AppBar(
        backgroundColor: AppTheme.surface,
        title: const Text(
          'OBD-II Connect',
          style: TextStyle(color: AppTheme.onBackground, fontWeight: FontWeight.w600),
        ),
        iconTheme: const IconThemeData(color: AppTheme.onSurface),
        actions: [
          IconButton(
            icon: _scanning
                ? const SizedBox(
                    width: 18, height: 18,
                    child: CircularProgressIndicator(strokeWidth: 2, color: AppTheme.accentOrange),
                  )
                : const Icon(Icons.refresh_rounded, color: AppTheme.accentOrange),
            onPressed: _scanning ? null : _scan,
            tooltip: 'Scan again',
          ),
        ],
      ),
      body: Column(
        children: [
          if (obdState.isConnected)
            _ConnectedBanner(
              address: obdState.deviceAddress,
              onDisconnect: () => ref.read(obdProvider.notifier).disconnect(),
            ),
          _InfoBox(),
          Expanded(
            child: _devices.isEmpty && !_scanning
                ? _EmptyState(onScan: _scan)
                : ListView.separated(
                    padding: const EdgeInsets.all(16),
                    itemCount: _devices.length,
                    separatorBuilder: (_, __) => const SizedBox(height: 8),
                    itemBuilder: (context, i) {
                      final d = _devices[i];
                      final isConnecting = _connectingAddress == d.address;
                      return _DeviceCard(
                        device:       d,
                        isConnecting: isConnecting,
                        isConnected:  obdState.isConnected &&
                                      obdState.deviceAddress == d.address,
                        onTap: () => _connect(d.address),
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }
}

class _ConnectedBanner extends StatelessWidget {
  const _ConnectedBanner({required this.address, required this.onDisconnect});
  final String    address;
  final VoidCallback onDisconnect;

  @override
  Widget build(BuildContext context) {
    return Container(
      color: AppTheme.accentGreen.withAlpha(25),
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      child: Row(
        children: [
          const Icon(Icons.bluetooth_connected, color: AppTheme.accentGreen, size: 18),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              'Connected to $address',
              style: const TextStyle(color: AppTheme.accentGreen, fontSize: 13),
            ),
          ),
          TextButton(
            onPressed: onDisconnect,
            child: const Text('Disconnect', style: TextStyle(color: AppTheme.accentRed)),
          ),
        ],
      ),
    );
  }
}

class _InfoBox extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.all(16),
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: AppTheme.surfaceVariant,
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: AppTheme.border),
      ),
      child: Row(
        children: [
          const Icon(Icons.info_outline, color: AppTheme.accentBlue, size: 18),
          const SizedBox(width: 10),
          const Expanded(
            child: Text(
              'Pair your ELM327 adapter via Android Bluetooth settings first, '
              'then select it below. OBD provides real RPM and throttle data.',
              style: TextStyle(color: AppTheme.onSurfaceDim, fontSize: 12),
            ),
          ),
        ],
      ),
    );
  }
}

class _EmptyState extends StatelessWidget {
  const _EmptyState({required this.onScan});
  final VoidCallback onScan;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Icon(Icons.bluetooth_searching, color: AppTheme.onSurfaceDim, size: 56),
          const SizedBox(height: 12),
          const Text('No paired devices found',
              style: TextStyle(color: AppTheme.onSurfaceDim, fontSize: 16)),
          const SizedBox(height: 6),
          const Text('Pair your ELM327 in Android Bluetooth settings',
              style: TextStyle(color: AppTheme.onSurfaceDim, fontSize: 12)),
          const SizedBox(height: 20),
          ElevatedButton.icon(
            onPressed: onScan,
            icon: const Icon(Icons.refresh),
            label: const Text('Scan Again'),
            style: ElevatedButton.styleFrom(
              backgroundColor: AppTheme.accentOrange,
              foregroundColor: Colors.white,
            ),
          ),
        ],
      ),
    );
  }
}

class _DeviceCard extends StatelessWidget {
  const _DeviceCard({
    required this.device,
    required this.isConnecting,
    required this.isConnected,
    required this.onTap,
  });
  final BTDeviceInfo device;
  final bool         isConnecting;
  final bool         isConnected;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final statusColor = isConnected   ? AppTheme.accentGreen
                      : isConnecting  ? AppTheme.accentOrange
                      : AppTheme.border;

    return GestureDetector(
      onTap: isConnected || isConnecting ? null : onTap,
      child: Container(
        padding: const EdgeInsets.all(14),
        decoration: BoxDecoration(
          color: AppTheme.surfaceVariant,
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: statusColor),
        ),
        child: Row(
          children: [
            Icon(
              isConnected ? Icons.bluetooth_connected : Icons.bluetooth,
              color: statusColor,
              size: 26,
            ),
            const SizedBox(width: 14),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(device.name,
                      style: const TextStyle(
                          color: AppTheme.onBackground,
                          fontWeight: FontWeight.w600,
                          fontSize: 15)),
                  const SizedBox(height: 2),
                  Text(device.address,
                      style: const TextStyle(
                          color: AppTheme.onSurfaceDim, fontSize: 12)),
                ],
              ),
            ),
            if (isConnecting)
              const SizedBox(
                width: 20, height: 20,
                child: CircularProgressIndicator(
                  strokeWidth: 2, color: AppTheme.accentOrange),
              )
            else if (isConnected)
              const Text('CONNECTED',
                  style: TextStyle(
                      color: AppTheme.accentGreen,
                      fontSize: 11, fontWeight: FontWeight.w700))
            else
              const Icon(Icons.chevron_right, color: AppTheme.onSurfaceDim),
          ],
        ),
      ),
    );
  }
}
