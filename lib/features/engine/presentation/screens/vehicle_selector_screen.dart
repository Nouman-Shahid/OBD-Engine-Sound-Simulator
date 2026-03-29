import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/core/theme/app_theme.dart';
import 'package:enginex/features/engine/data/models/vehicle_profile.dart';
import 'package:enginex/features/engine/domain/engine_providers.dart';
import 'package:enginex/features/engine/presentation/screens/engine_screen.dart';

class VehicleSelectorScreen extends ConsumerWidget {
  const VehicleSelectorScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final vehicles = ref.watch(vehicleCatalogueProvider);

    return Scaffold(
      backgroundColor: AppTheme.background,
      body: SafeArea(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const SizedBox(height: 28),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'ENGINEX PRO',
                    style: Theme.of(context).textTheme.labelSmall?.copyWith(
                      color: AppTheme.accentOrange,
                      fontSize: 12,
                      letterSpacing: 4,
                      fontWeight: FontWeight.w700,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    'Select\nVehicle',
                    style: Theme.of(context).textTheme.displayLarge?.copyWith(
                      fontSize: 48,
                      height: 1.05,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    '${vehicles.length} powertrain profiles — real-time synthesis.',
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: AppTheme.onSurfaceDim,
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 20),
            Expanded(
              child: ListView.separated(
                padding: const EdgeInsets.fromLTRB(16, 0, 16, 24),
                itemCount: vehicles.length,
                separatorBuilder: (_, __) => const SizedBox(height: 8),
                itemBuilder: (context, i) => _VehicleCard(
                  profile: vehicles[i],
                  onTap: () => _navigate(context, vehicles[i]),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  void _navigate(BuildContext context, VehicleProfile p) {
    Navigator.of(context).push(
      PageRouteBuilder<void>(
        pageBuilder: (_, anim, __) => EngineScreen(vehicle: p),
        transitionsBuilder: (_, anim, __, child) => FadeTransition(
          opacity: anim,
          child: SlideTransition(
            position: Tween<Offset>(
              begin: const Offset(0, 0.04),
              end: Offset.zero,
            ).animate(CurvedAnimation(parent: anim, curve: Curves.easeOutCubic)),
            child: child,
          ),
        ),
        transitionDuration: const Duration(milliseconds: 320),
      ),
    );
  }
}

class _VehicleCard extends StatelessWidget {
  const _VehicleCard({required this.profile, required this.onTap});
  final VehicleProfile profile;
  final VoidCallback   onTap;

  @override
  Widget build(BuildContext context) {
    final hasTurbo = profile.turboGain > 0;

    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.all(14),
        decoration: BoxDecoration(
          color: AppTheme.surfaceVariant,
          borderRadius: BorderRadius.circular(14),
          border: Border.all(color: AppTheme.border),
        ),
        child: Row(
          children: [
            // Icon
            Container(
              width: 56, height: 56,
              decoration: BoxDecoration(
                color: AppTheme.surface,
                borderRadius: BorderRadius.circular(12),
                border: Border.all(color: AppTheme.border),
              ),
              alignment: Alignment.center,
              child: Text(profile.emoji, style: const TextStyle(fontSize: 26)),
            ),
            const SizedBox(width: 14),
            // Info
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Text(profile.name,
                          style: Theme.of(context).textTheme.titleLarge),
                      if (hasTurbo) ...[
                        const SizedBox(width: 6),
                        Container(
                          padding: const EdgeInsets.symmetric(
                              horizontal: 5, vertical: 2),
                          decoration: BoxDecoration(
                            color: AppTheme.accentBlue.withAlpha(30),
                            borderRadius: BorderRadius.circular(4),
                            border: Border.all(
                                color: AppTheme.accentBlue.withAlpha(100)),
                          ),
                          child: const Text('TURBO',
                              style: TextStyle(
                                  color: AppTheme.accentBlue,
                                  fontSize: 9,
                                  fontWeight: FontWeight.w700,
                                  letterSpacing: 0.5)),
                        ),
                      ],
                    ],
                  ),
                  const SizedBox(height: 2),
                  Text(
                    profile.description,
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: AppTheme.onSurfaceDim,
                      fontSize: 12,
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                  const SizedBox(height: 6),
                  // Spec pills
                  Row(
                    children: [
                      _SpecPill('${profile.cylinders} CYL', AppTheme.accentBlue),
                      const SizedBox(width: 5),
                      _SpecPill(
                        '${(profile.revLimiterRpm / 1000).toStringAsFixed(1)}k RPM',
                        AppTheme.accentOrange,
                      ),
                      const SizedBox(width: 5),
                      _SpecPill(
                        '${profile.gearRatios.length - 1} SPD',
                        AppTheme.accentGreen,
                      ),
                    ],
                  ),
                ],
              ),
            ),
            const SizedBox(width: 6),
            const Icon(Icons.chevron_right_rounded, color: AppTheme.onSurfaceDim),
          ],
        ),
      ),
    );
  }
}

class _SpecPill extends StatelessWidget {
  const _SpecPill(this.label, this.color);
  final String label;
  final Color  color;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 7, vertical: 2),
      decoration: BoxDecoration(
        color: color.withAlpha(25),
        borderRadius: BorderRadius.circular(5),
        border: Border.all(color: color.withAlpha(100)),
      ),
      child: Text(
        label,
        style: TextStyle(
            color: color, fontSize: 10,
            fontWeight: FontWeight.w700, letterSpacing: 0.4),
      ),
    );
  }
}
