import 'dart:math' as math;

import 'package:flutter/material.dart';

import 'package:enginex/core/constants/app_constants.dart';
import 'package:enginex/core/theme/app_theme.dart';
import 'package:enginex/features/engine/data/models/engine_state.dart';

/// Analogue RPM gauge rendered entirely via CustomPainter.
/// Designed to animate at 60 fps with zero widget rebuilds beyond
/// the AnimatedBuilder wrapper — the painter does all the work.
class RpmGauge extends StatefulWidget {
  const RpmGauge({
    super.key,
    required this.rpm,
    required this.maxRpm,
    required this.isRunning,
    required this.isRevLimiting,
    this.size = AppConstants.gaugeSize,
  });

  final double rpm;
  final double maxRpm;
  final bool   isRunning;
  final bool   isRevLimiting;
  final double size;

  @override
  State<RpmGauge> createState() => _RpmGaugeState();
}

class _RpmGaugeState extends State<RpmGauge>
    with SingleTickerProviderStateMixin {
  late final AnimationController _needle;
  late Animation<double> _needleAnim;
  double _prevRpm = 0.0;

  @override
  void initState() {
    super.initState();
    _needle = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 80),
    );
    _needleAnim = Tween<double>(begin: 0, end: 0).animate(
      CurvedAnimation(parent: _needle, curve: Curves.easeOut),
    );
  }

  @override
  void didUpdateWidget(RpmGauge old) {
    super.didUpdateWidget(old);
    if ((widget.rpm - _prevRpm).abs() > 5) {
      _needleAnim = Tween<double>(
        begin: _prevRpm / widget.maxRpm,
        end: widget.rpm / widget.maxRpm,
      ).animate(CurvedAnimation(parent: _needle, curve: Curves.easeOut));
      _needle.forward(from: 0);
      _prevRpm = widget.rpm;
    }
  }

  @override
  void dispose() {
    _needle.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _needle,
      builder: (context, _) => CustomPaint(
        size: Size(widget.size, widget.size),
        painter: _GaugePainter(
          normalizedRpm: _needleAnim.value,
          maxRpm:        widget.maxRpm,
          isRunning:     widget.isRunning,
          isRevLimiting: widget.isRevLimiting,
        ),
      ),
    );
  }
}

// ── Painter ────────────────────────────────────────────────────────────────

class _GaugePainter extends CustomPainter {
  _GaugePainter({
    required this.normalizedRpm,
    required this.maxRpm,
    required this.isRunning,
    required this.isRevLimiting,
  });

  final double normalizedRpm;
  final double maxRpm;
  final bool   isRunning;
  final bool   isRevLimiting;

  // The gauge sweeps from -225° to +45° (270° total arc).
  static const double _startAngle = -225 * (math.pi / 180);
  static const double _sweepTotal =  270 * (math.pi / 180);

  Color get _needleColor {
    if (!isRunning) return AppTheme.onSurfaceDim;
    if (isRevLimiting) return AppTheme.rpmDanger;
    if (normalizedRpm > 0.875) return AppTheme.rpmDanger;
    if (normalizedRpm > 0.75)  return AppTheme.rpmHigh;
    if (normalizedRpm > 0.5)   return AppTheme.rpmMid;
    if (normalizedRpm > 0.25)  return AppTheme.rpmLow;
    return AppTheme.rpmIdle;
  }

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2;

    _drawBackground(canvas, center, radius);
    _drawArcZones(canvas, center, radius);
    _drawTicks(canvas, center, radius);
    _drawLabels(canvas, center, radius);
    _drawNeedle(canvas, center, radius);
    _drawHub(canvas, center);
  }

  void _drawBackground(Canvas canvas, Offset center, double radius) {
    final paint = Paint()
      ..color = AppTheme.surface
      ..style = PaintingStyle.fill;
    canvas.drawCircle(center, radius, paint);

    final borderPaint = Paint()
      ..color = AppTheme.border
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.5;
    canvas.drawCircle(center, radius - 1, borderPaint);

    // Inner ring shadow.
    final innerShadow = Paint()
      ..color = Colors.black.withAlpha(120)
      ..style = PaintingStyle.fill;
    canvas.drawCircle(center, radius * 0.72, innerShadow);
  }

  void _drawArcZones(Canvas canvas, Offset center, double radius) {
    final arcRect = Rect.fromCircle(center: center, radius: radius * 0.84);
    const strokeW = 10.0;

    void drawZone(double startN, double endN, Color color) {
      final paint = Paint()
        ..color = color.withAlpha(isRunning ? 200 : 80)
        ..style = PaintingStyle.stroke
        ..strokeWidth = strokeW
        ..strokeCap = StrokeCap.butt;
      canvas.drawArc(
        arcRect,
        _startAngle + startN * _sweepTotal,
        (endN - startN) * _sweepTotal,
        false,
        paint,
      );
    }

    drawZone(0.0,   0.25,  AppTheme.rpmIdle);
    drawZone(0.25,  0.50,  AppTheme.rpmLow);
    drawZone(0.50,  0.75,  AppTheme.rpmMid);
    drawZone(0.75,  0.875, AppTheme.rpmHigh);
    drawZone(0.875, 1.0,   AppTheme.rpmDanger);
  }

  void _drawTicks(Canvas canvas, Offset center, double radius) {
    const totalTicks = 80;
    for (int i = 0; i <= totalTicks; i++) {
      final t = i / totalTicks;
      final angle = _startAngle + t * _sweepTotal;
      final isMajor = i % 8 == 0;
      final isMinor = i % 4 == 0;

      final outerR = radius * 0.84;
      final innerR = isMajor
          ? radius * 0.68
          : isMinor
              ? radius * 0.74
              : radius * 0.79;

      final cos = math.cos(angle);
      final sin = math.sin(angle);

      final p1 = Offset(center.dx + outerR * cos, center.dy + outerR * sin);
      final p2 = Offset(center.dx + innerR * cos, center.dy + innerR * sin);

      canvas.drawLine(
        p1, p2,
        Paint()
          ..color = isMajor ? AppTheme.onSurface : AppTheme.onSurfaceDim
          ..strokeWidth = isMajor ? 2.0 : 1.0,
      );
    }
  }

  void _drawLabels(Canvas canvas, Offset center, double radius) {
    final maxK = (maxRpm / 1000).round();
    for (int k = 0; k <= maxK; k++) {
      final t = k / maxK;
      final angle = _startAngle + t * _sweepTotal;
      final labelR = radius * 0.58;

      final tp = TextPainter(
        text: TextSpan(
          text: '$k',
          style: TextStyle(
            color: AppTheme.onSurfaceDim,
            fontSize: radius * 0.085,
            fontWeight: FontWeight.w600,
          ),
        ),
        textDirection: TextDirection.ltr,
      )..layout();

      final pos = Offset(
        center.dx + labelR * math.cos(angle) - tp.width / 2,
        center.dy + labelR * math.sin(angle) - tp.height / 2,
      );
      tp.paint(canvas, pos);
    }

    // "RPM x1000" label.
    final unitTp = TextPainter(
      text: const TextSpan(
        text: 'RPM ×1000',
        style: TextStyle(color: AppTheme.onSurfaceDim, fontSize: 11),
      ),
      textDirection: TextDirection.ltr,
    )..layout();
    unitTp.paint(
      canvas,
      Offset(center.dx - unitTp.width / 2, center.dy + radius * 0.35),
    );
  }

  void _drawNeedle(Canvas canvas, Offset center, double radius) {
    final angle = _startAngle + normalizedRpm.clamp(0.0, 1.0) * _sweepTotal;
    final needleLength = radius * 0.75;
    final tailLength   = radius * 0.18;

    final tip  = Offset(
      center.dx + needleLength * math.cos(angle),
      center.dy + needleLength * math.sin(angle),
    );
    final tail = Offset(
      center.dx - tailLength * math.cos(angle),
      center.dy - tailLength * math.sin(angle),
    );

    // Glow.
    canvas.drawLine(
      tail, tip,
      Paint()
        ..color = _needleColor.withAlpha(60)
        ..strokeWidth = 8
        ..strokeCap = StrokeCap.round
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 6),
    );

    // Needle body.
    canvas.drawLine(
      tail, tip,
      Paint()
        ..color = _needleColor
        ..strokeWidth = 2.5
        ..strokeCap = StrokeCap.round,
    );
  }

  void _drawHub(Canvas canvas, Offset center) {
    canvas.drawCircle(center, 12, Paint()..color = AppTheme.accentOrange);
    canvas.drawCircle(center, 6,  Paint()..color = AppTheme.background);
  }

  @override
  bool shouldRepaint(_GaugePainter old) =>
      old.normalizedRpm != normalizedRpm ||
      old.isRunning     != isRunning     ||
      old.isRevLimiting != isRevLimiting;
}
