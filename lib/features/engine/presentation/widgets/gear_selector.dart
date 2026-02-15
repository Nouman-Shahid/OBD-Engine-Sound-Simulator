import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'package:enginex/core/theme/app_theme.dart';

/// H-pattern gear selector widget.
/// Supports neutral (N) plus gears 1–6 with haptic feedback.
class GearSelector extends StatelessWidget {
  const GearSelector({
    super.key,
    required this.currentGear,
    required this.onGearChanged,
    required this.enabled,
  });

  final int currentGear;
  final ValueChanged<int> onGearChanged;
  final bool enabled;

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Text(
          'GEAR',
          style: Theme.of(context).textTheme.labelSmall?.copyWith(
            letterSpacing: 2.0,
            color: AppTheme.onSurfaceDim,
          ),
        ),
        const SizedBox(height: 8),
        // Gear display.
        Container(
          width: 64,
          height: 64,
          decoration: BoxDecoration(
            color: AppTheme.surfaceVariant,
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: currentGear == 0
                  ? AppTheme.border
                  : AppTheme.accentOrange,
              width: 2,
            ),
            boxShadow: currentGear > 0
                ? [
                    BoxShadow(
                      color: AppTheme.accentOrange.withAlpha(80),
                      blurRadius: 12,
                    )
                  ]
                : null,
          ),
          alignment: Alignment.center,
          child: Text(
            currentGear == 0 ? 'N' : '$currentGear',
            style: Theme.of(context).textTheme.displayLarge?.copyWith(
              fontSize: 36,
              fontWeight: FontWeight.w800,
              color: currentGear == 0
                  ? AppTheme.onSurfaceDim
                  : AppTheme.accentOrange,
            ),
          ),
        ),
        const SizedBox(height: 12),
        // Shift buttons.
        Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            _ShiftButton(
              label: '−',
              onTap: enabled && currentGear > 0
                  ? () {
                      HapticFeedback.lightImpact();
                      onGearChanged(currentGear - 1);
                    }
                  : null,
            ),
            const SizedBox(width: 8),
            _ShiftButton(
              label: '+',
              onTap: enabled && currentGear < 6
                  ? () {
                      HapticFeedback.mediumImpact();
                      onGearChanged(currentGear + 1);
                    }
                  : null,
            ),
          ],
        ),
        const SizedBox(height: 12),
        // Mini gear row.
        Wrap(
          spacing: 4,
          children: [
            for (int g = 0; g <= 6; g++)
              _MiniGearDot(
                label: g == 0 ? 'N' : '$g',
                isActive: g == currentGear,
                onTap: enabled ? () {
                  HapticFeedback.selectionClick();
                  onGearChanged(g);
                } : null,
              ),
          ],
        ),
      ],
    );
  }
}

class _ShiftButton extends StatelessWidget {
  const _ShiftButton({required this.label, required this.onTap});
  final String label;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    final enabled = onTap != null;
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 150),
        width: 44,
        height: 44,
        decoration: BoxDecoration(
          color: enabled ? AppTheme.surfaceVariant : AppTheme.surface,
          borderRadius: BorderRadius.circular(10),
          border: Border.all(
            color: enabled ? AppTheme.accentOrange : AppTheme.border,
          ),
        ),
        alignment: Alignment.center,
        child: Text(
          label,
          style: TextStyle(
            fontSize: 24,
            fontWeight: FontWeight.w700,
            color: enabled ? AppTheme.accentOrange : AppTheme.onSurfaceDim,
          ),
        ),
      ),
    );
  }
}

class _MiniGearDot extends StatelessWidget {
  const _MiniGearDot({
    required this.label,
    required this.isActive,
    required this.onTap,
  });

  final String label;
  final bool   isActive;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 120),
        width: 30,
        height: 30,
        decoration: BoxDecoration(
          color: isActive ? AppTheme.accentOrange : AppTheme.surfaceVariant,
          borderRadius: BorderRadius.circular(6),
          border: Border.all(
            color: isActive ? AppTheme.accentOrange : AppTheme.border,
          ),
        ),
        alignment: Alignment.center,
        child: Text(
          label,
          style: TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.w700,
            color: isActive ? Colors.white : AppTheme.onSurfaceDim,
          ),
        ),
      ),
    );
  }
}
