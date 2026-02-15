import 'package:flutter/material.dart';

import 'package:enginex/core/theme/app_theme.dart';

/// Vertical throttle pedal slider with real-time visual feedback.
/// Uses a [GestureDetector] on top of a custom painter for precise
/// drag tracking — the slider reports values in [0, 1].
class ThrottleSlider extends StatefulWidget {
  const ThrottleSlider({
    super.key,
    required this.value,
    required this.onChanged,
    this.enabled = true,
  });

  final double value;
  final ValueChanged<double> onChanged;
  final bool enabled;

  @override
  State<ThrottleSlider> createState() => _ThrottleSliderState();
}

class _ThrottleSliderState extends State<ThrottleSlider>
    with SingleTickerProviderStateMixin {
  late AnimationController _glow;
  final GlobalKey _trackKey = GlobalKey();

  @override
  void initState() {
    super.initState();
    _glow = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 400),
    );
  }

  @override
  void didUpdateWidget(ThrottleSlider old) {
    super.didUpdateWidget(old);
    if (widget.value > 0.05 && old.value <= 0.05) {
      _glow.forward();
    } else if (widget.value <= 0.05 && old.value > 0.05) {
      _glow.reverse();
    }
  }

  @override
  void dispose() {
    _glow.dispose();
    super.dispose();
  }

  double _computeValue(Offset localPos, double trackHeight) {
    final raw = 1.0 - (localPos.dy / trackHeight);
    return raw.clamp(0.0, 1.0);
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Text(
          'THROTTLE',
          style: Theme.of(context).textTheme.labelSmall?.copyWith(
            letterSpacing: 2.0,
            color: AppTheme.onSurfaceDim,
          ),
        ),
        const SizedBox(height: 8),
        Text(
          '${(widget.value * 100).round()}%',
          style: Theme.of(context).textTheme.titleLarge?.copyWith(
            color: _throttleColor(widget.value),
            fontWeight: FontWeight.w700,
          ),
        ),
        const SizedBox(height: 12),
        LayoutBuilder(
          builder: (context, constraints) {
            const trackW = 52.0;
            const trackH = 220.0;
            return GestureDetector(
              behavior: HitTestBehavior.opaque,
              onPanUpdate: widget.enabled
                  ? (d) {
                      final box = _trackKey.currentContext?.findRenderObject()
                          as RenderBox?;
                      if (box == null) return;
                      final local = box.globalToLocal(d.globalPosition);
                      widget.onChanged(_computeValue(local, trackH));
                    }
                  : null,
              onTapUp: widget.enabled
                  ? (d) {
                      final box = _trackKey.currentContext?.findRenderObject()
                          as RenderBox?;
                      if (box == null) return;
                      final local = box.globalToLocal(d.globalPosition);
                      widget.onChanged(_computeValue(local, trackH));
                    }
                  : null,
              child: AnimatedBuilder(
                animation: _glow,
                builder: (context, _) => CustomPaint(
                  key: _trackKey,
                  size: const Size(trackW, trackH),
                  painter: _ThrottlePainter(
                    value:    widget.value,
                    glowAmt:  _glow.value,
                    enabled:  widget.enabled,
                  ),
                ),
              ),
            );
          },
        ),
      ],
    );
  }

  Color _throttleColor(double v) {
    if (v < 0.3) return AppTheme.rpmIdle;
    if (v < 0.6) return AppTheme.rpmLow;
    if (v < 0.8) return AppTheme.rpmMid;
    if (v < 0.95) return AppTheme.rpmHigh;
    return AppTheme.rpmDanger;
  }
}

class _ThrottlePainter extends CustomPainter {
  _ThrottlePainter({
    required this.value,
    required this.glowAmt,
    required this.enabled,
  });

  final double value;
  final double glowAmt;
  final bool   enabled;

  @override
  void paint(Canvas canvas, Size size) {
    const radius = Radius.circular(8);
    final trackRect = RRect.fromLTRBR(0, 0, size.width, size.height, radius);

    // Track background.
    canvas.drawRRect(trackRect, Paint()..color = AppTheme.surfaceVariant);
    canvas.drawRRect(
      trackRect,
      Paint()
        ..color = AppTheme.border
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1,
    );

    if (!enabled) return;

    final fillH = size.height * value;
    final fillY = size.height - fillH;

    // Gradient fill.
    final fillRect = RRect.fromLTRBR(
      0, fillY, size.width, size.height, radius,
    );
    canvas.drawRRect(
      fillRect,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.bottomCenter,
          end: Alignment.topCenter,
          colors: [
            AppTheme.accentOrange,
            AppTheme.rpmMid,
          ],
        ).createShader(fillRect.outerRect),
    );

    // Glow overlay.
    if (glowAmt > 0) {
      canvas.drawRRect(
        fillRect,
        Paint()
          ..color = AppTheme.accentOrange.withAlpha((glowAmt * 60).round())
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 12),
      );
    }

    // Thumb line.
    final thumbY = fillY;
    canvas.drawLine(
      Offset(4, thumbY),
      Offset(size.width - 4, thumbY),
      Paint()
        ..color = Colors.white.withAlpha(200)
        ..strokeWidth = 2
        ..strokeCap = StrokeCap.round,
    );
  }

  @override
  bool shouldRepaint(_ThrottlePainter old) =>
      old.value != value || old.glowAmt != glowAmt || old.enabled != enabled;
}
