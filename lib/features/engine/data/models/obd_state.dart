/// OBD-II connection and data state.
///
/// Parsed from native events on the `com.enginex/obd_event` EventChannel.
class OBDState {
  const OBDState({
    this.isConnected  = false,
    this.deviceAddress = '',
    this.rpm           = 0.0,
    this.throttle      = 0.0,
    this.errorMessage  = '',
  });

  final bool   isConnected;
  final String deviceAddress;
  final double rpm;
  final double throttle;
  final String errorMessage;

  bool get hasError => errorMessage.isNotEmpty;

  OBDState copyWith({
    bool?   isConnected,
    String? deviceAddress,
    double? rpm,
    double? throttle,
    String? errorMessage,
  }) => OBDState(
    isConnected   : isConnected   ?? this.isConnected,
    deviceAddress : deviceAddress ?? this.deviceAddress,
    rpm           : rpm           ?? this.rpm,
    throttle      : throttle      ?? this.throttle,
    errorMessage  : errorMessage  ?? this.errorMessage,
  );

  static const disconnected = OBDState();
}

/// A Bluetooth device entry from [OBDService.scanPairedDevices].
class BTDeviceInfo {
  const BTDeviceInfo({required this.name, required this.address});
  final String name;
  final String address;

  static BTDeviceInfo fromMap(Map<String, dynamic> m) =>
      BTDeviceInfo(name: m['name'] as String, address: m['address'] as String);
}
