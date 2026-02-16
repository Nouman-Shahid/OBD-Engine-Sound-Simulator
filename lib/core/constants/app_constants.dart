library;

class AppConstants {
  AppConstants._();

  // ── Engine limits ──────────────────────────────────────────────────────────
  static const double idleRpm       = 800.0;
  static const double revLimiterRpm = 7500.0;
  static const double maxDisplayRpm = 8000.0;

  // ── Gear definitions ──────────────────────────────────────────────────────
  static const int neutralGear = 0;
  static const int maxGear     = 6;

  // ── UI constants ──────────────────────────────────────────────────────────
  static const double gaugeSize       = 280.0;
  static const int   rpmUpdateHz      = 60;
  static const Duration rpmUpdateInterval =
      Duration(milliseconds: 1000 ~/ rpmUpdateHz);

  // ── Audio layers ──────────────────────────────────────────────────────────
  static const double idleRpmThreshold  = 1200.0;
  static const double lowRpmThreshold   = 2800.0;
  static const double midRpmThreshold   = 4500.0;
  static const double highRpmThreshold  = 6500.0;
}
