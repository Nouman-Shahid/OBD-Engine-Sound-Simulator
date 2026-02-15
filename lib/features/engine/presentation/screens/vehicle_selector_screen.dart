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
            const SizedBox(height: 32),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'ENGINEX',
                    style: Theme.of(context).textTheme.labelSmall?.copyWith(
                      color: AppTheme.accentOrange,
                      fontSize: 13,
                      letterSpacing: 4,
                      fontWeight: FontWeight.w700,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    'Select\nVehicle',
                    style: Theme.of(context).textTheme.displayLarge?.copyWith(
                      fontSize: 52,
                      height: 1.05,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    'Choose a powertrain profile to simulate.',
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: AppTheme.onSurfaceDim,
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 32),
            Expanded(
              child: ListView.separated(
                padding: const EdgeInsets.fromLTRB(16, 0, 16, 24),
                itemCount: vehicles.length,
                separatorBuilder: (_, __) => const SizedBox(height: 10),
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
        transitionDuration: const Duration(milliseconds: 350),
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
    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: AppTheme.surfaceVariant,
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: AppTheme.border),
        ),
        child: Row(
          children: [
            // Icon circle.
            Container(
              width: 60,
              height: 60,
              decoration: BoxDecoration(
                color: AppTheme.surface,
                borderRadius: BorderRadius.circular(14),
                border: Border.all(color: AppTheme.border),
              ),
              alignment: Alignment.center,
              child: Text(
                profile.emoji,
                style: const TextStyle(fontSize: 28),
              ),
            ),
            const SizedBox(width: 16),
            // Info.
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    profile.name,
                    style: Theme.of(context).textTheme.titleLarge,
                  ),
                  const SizedBox(height: 2),
                  Text(
                    profile.description,
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: AppTheme.onSurfaceDim,
                      fontSize: 13,
                    ),
                  ),
                  const SizedBox(height: 8),
                  // Spec pills.
                  Row(
                    children: [
                      _SpecPill(
                        label: '${profile.cylinders} CYL',
                        color: AppTheme.accentBlue,
                      ),
                      const SizedBox(width: 6),
                      _SpecPill(
                        label: '${(profile.revLimiterRpm / 1000).toStringAsFixed(1)}k RPM',
                        color: AppTheme.accentOrange,
                      ),
                      const SizedBox(width: 6),
                      _SpecPill(
                        label: '${profile.gearRatios.length - 1} SPD',
                        color: AppTheme.accentGreen,
                      ),
                    ],
                  ),
                ],
              ),
            ),
            const SizedBox(width: 8),
            const Icon(
              Icons.chevron_right_rounded,
              color: AppTheme.onSurfaceDim,
            ),
          ],
        ),
      ),
    );
  }
}

class _SpecPill extends StatelessWidget {
  const _SpecPill({required this.label, required this.color});
  final String label;
  final Color  color;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
      decoration: BoxDecoration(
        color: color.withAlpha(30),
        borderRadius: BorderRadius.circular(6),
        border: Border.all(color: color.withAlpha(120)),
      ),
      child: Text(
        label,
        style: TextStyle(
          color: color,
          fontSize: 11,
          fontWeight: FontWeight.w700,
          letterSpacing: 0.5,
        ),
      ),
    );
  }
}
