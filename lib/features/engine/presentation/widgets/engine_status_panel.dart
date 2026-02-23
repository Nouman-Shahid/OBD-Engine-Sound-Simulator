import 'package:flutter/material.dart';

import 'package:enginex/core/theme/app_theme.dart';
import 'package:enginex/features/engine/data/models/engine_state.dart';

/// Compact status bar — shows active audio layer, rev-limit alert, and
/// a blinking engine-running indicator.
class EngineStatusPanel extends StatefulWidget {
  const EngineStatusPanel({
    super.key,
    required this.state,
  });

  final EngineState state;

  @override
  State<EngineStatusPanel> createState() => _EngineStatusPanelState();
}

class _EngineStatusPanelState extends State<EngineStatusPanel>
    with SingleTickerProviderStateMixin {
  late AnimationController _blink;

  @override
  void initState() {
    super.initState();
    _blink = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 600),
    )..repeat(reverse: true);
  }

  @override
  void dispose() {
    _blink.dispose();
    super.dispose();
  }

  String _layerLabel(AudioLayer layer) {
    switch (layer) {
      case AudioLayer.idle: return 'IDLE';
      case AudioLayer.low:  return 'LOW RPM';
      case AudioLayer.mid:  return 'MID RPM';
      case AudioLayer.high: return 'HIGH RPM';
    }
  }

  Color _layerColor(AudioLayer layer) {
    switch (layer) {
      case AudioLayer.idle: return AppTheme.rpmIdle;
      case AudioLayer.low:  return AppTheme.rpmLow;
      case AudioLayer.mid:  return AppTheme.rpmMid;
      case AudioLayer.high: return AppTheme.rpmHigh;
    }
  }

  @override
  Widget build(BuildContext context) {
    final s = widget.state;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
      decoration: BoxDecoration(
        color: AppTheme.surfaceVariant,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: AppTheme.border),
      ),
      child: Row(
        children: [
          // Running indicator.
          AnimatedBuilder(
            animation: _blink,
            builder: (_, __) => Container(
              width: 8,
              height: 8,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: s.isRunning
                    ? AppTheme.accentGreen.withAlpha(
                        ((_blink.value * 100) + 155).round(),
                      )
                    : AppTheme.onSurfaceDim,
              ),
            ),
          ),
          const SizedBox(width: 8),
          Text(
            s.isRunning ? 'RUNNING' : 'STOPPED',
            style: Theme.of(context).textTheme.labelSmall?.copyWith(
              color: s.isRunning ? AppTheme.accentGreen : AppTheme.onSurfaceDim,
              letterSpacing: 1.5,
            ),
          ),
          const Spacer(),

          // Layer badge.
          if (s.isRunning) ...[
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
              decoration: BoxDecoration(
                color: _layerColor(s.activeLayer).withAlpha(30),
                borderRadius: BorderRadius.circular(6),
                border: Border.all(
                  color: _layerColor(s.activeLayer).withAlpha(120),
                ),
              ),
              child: Text(
                _layerLabel(s.activeLayer),
                style: Theme.of(context).textTheme.labelSmall?.copyWith(
                  color: _layerColor(s.activeLayer),
                  letterSpacing: 1.2,
                ),
              ),
            ),
            const SizedBox(width: 8),
          ],

          // Rev limit badge.
          if (s.isRevLimiting)
            AnimatedBuilder(
              animation: _blink,
              builder: (_, __) => Opacity(
                opacity: 0.4 + _blink.value * 0.6,
                child: Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                  decoration: BoxDecoration(
                    color: AppTheme.rpmDanger.withAlpha(40),
                    borderRadius: BorderRadius.circular(6),
                    border: Border.all(color: AppTheme.rpmDanger),
                  ),
                  child: Text(
                    'REV LIMIT',
                    style: Theme.of(context).textTheme.labelSmall?.copyWith(
                      color: AppTheme.rpmDanger,
                      letterSpacing: 1.2,
                      fontWeight: FontWeight.w800,
                    ),
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }
}
