import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/core/theme/app_theme.dart';
import 'package:enginex/features/engine/domain/engine_providers.dart';

/// Compact OBD status badge for the engine screen header.
///
/// Shows:
///  - Grey   "OBD" pill when disconnected
///  - Green  "OBD LIVE" pill when connected + streaming data
///
/// Tapping it opens the OBD connect screen.
class OBDStatusBadge extends ConsumerWidget {
  const OBDStatusBadge({super.key, required this.onTap});
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final isConnected = ref.watch(isObdConnectedProvider);

    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 300),
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
        decoration: BoxDecoration(
          color: isConnected
              ? AppTheme.accentGreen.withAlpha(30)
              : AppTheme.surfaceVariant,
          borderRadius: BorderRadius.circular(8),
          border: Border.all(
            color: isConnected ? AppTheme.accentGreen : AppTheme.border,
          ),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            AnimatedContainer(
              duration: const Duration(milliseconds: 300),
              width: 6, height: 6,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: isConnected ? AppTheme.accentGreen : AppTheme.onSurfaceDim,
              ),
            ),
            const SizedBox(width: 5),
            Text(
              isConnected ? 'OBD LIVE' : 'OBD',
              style: TextStyle(
                color: isConnected ? AppTheme.accentGreen : AppTheme.onSurfaceDim,
                fontSize: 10,
                fontWeight: FontWeight.w700,
                letterSpacing: 0.8,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
