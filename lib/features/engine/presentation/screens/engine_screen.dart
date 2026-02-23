import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:google_fonts/google_fonts.dart';

import 'package:enginex/core/theme/app_theme.dart';
import 'package:enginex/features/engine/data/models/vehicle_profile.dart';
import 'package:enginex/features/engine/domain/engine_providers.dart';
import 'package:enginex/features/engine/presentation/screens/vehicle_selector_screen.dart';
import 'package:enginex/features/engine/presentation/widgets/engine_status_panel.dart';
import 'package:enginex/features/engine/presentation/widgets/gear_selector.dart';
import 'package:enginex/features/engine/presentation/widgets/rpm_gauge.dart';
import 'package:enginex/features/engine/presentation/widgets/throttle_slider.dart';

class EngineScreen extends ConsumerStatefulWidget {
  const EngineScreen({super.key, required this.vehicle});
  final VehicleProfile vehicle;

  @override
  ConsumerState<EngineScreen> createState() => _EngineScreenState();
}

class _EngineScreenState extends ConsumerState<EngineScreen>
    with WidgetsBindingObserver {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    // Select the vehicle that was passed from the selector screen.
    WidgetsBinding.instance.addPostFrameCallback((_) {
      ref.read(engineProvider.notifier).selectVehicle(widget.vehicle);
    });
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.paused ||
        state == AppLifecycleState.detached) {
      ref.read(engineProvider.notifier).stopEngine();
    }
  }

  @override
  Widget build(BuildContext context) {
    final engine    = ref.watch(engineProvider);
    final notifier  = ref.read(engineProvider.notifier);
    final isRunning = engine.isRunning;

    return Scaffold(
      backgroundColor: AppTheme.background,
      body: SafeArea(
        child: Column(
          children: [
            _buildHeader(context, notifier, isRunning),
            const SizedBox(height: 8),
            EngineStatusPanel(state: engine),
            const SizedBox(height: 16),
            // ── RPM Gauge ──────────────────────────────────────────────────
            Center(
              child: RpmGauge(
                rpm:           engine.rpm,
                maxRpm:        widget.vehicle.maxRpm,
                isRunning:     isRunning,
                isRevLimiting: engine.isRevLimiting,
              ),
            ),
            const SizedBox(height: 8),
            // Numeric RPM readout.
            _RpmReadout(rpm: engine.rpm, isRunning: isRunning),
            const Spacer(),
            // ── Controls row ───────────────────────────────────────────────
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.end,
                children: [
                  ThrottleSlider(
                    value:     engine.throttle,
                    onChanged: isRunning ? notifier.setThrottle : (_) {},
                    enabled:   isRunning,
                  ),
                  const Spacer(),
                  GearSelector(
                    currentGear:   engine.gear,
                    onGearChanged: notifier.setGear,
                    enabled:       isRunning,
                  ),
                ],
              ),
            ),
            const SizedBox(height: 24),
            // ── Start / Stop button ────────────────────────────────────────
            _StartStopButton(
              isRunning: isRunning,
              onTap: () {
                if (isRunning) {
                  notifier.stopEngine();
                } else {
                  notifier.startEngine();
                }
              },
            ),
            const SizedBox(height: 32),
          ],
        ),
      ),
    );
  }

  Widget _buildHeader(
    BuildContext context,
    dynamic notifier,
    bool isRunning,
  ) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(20, 12, 20, 0),
      child: Row(
        children: [
          // Back to selector.
          GestureDetector(
            onTap: () async {
              if (isRunning) {
                await ref.read(engineProvider.notifier).stopEngine();
              }
              if (context.mounted) {
                Navigator.of(context).pushReplacement(
                  MaterialPageRoute<void>(
                    builder: (_) => const VehicleSelectorScreen(),
                  ),
                );
              }
            },
            child: Container(
              padding: const EdgeInsets.all(8),
              decoration: BoxDecoration(
                color: AppTheme.surfaceVariant,
                borderRadius: BorderRadius.circular(10),
                border: Border.all(color: AppTheme.border),
              ),
              child: const Icon(
                Icons.arrow_back_ios_new_rounded,
                color: AppTheme.onSurface,
                size: 18,
              ),
            ),
          ),
          const SizedBox(width: 12),
          Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                widget.vehicle.name,
                style: Theme.of(context).textTheme.titleLarge,
              ),
              Text(
                widget.vehicle.description,
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  color: AppTheme.onSurfaceDim,
                  fontSize: 12,
                ),
              ),
            ],
          ),
          const Spacer(),
          Text(
            widget.vehicle.emoji,
            style: const TextStyle(fontSize: 32),
          ),
        ],
      ),
    );
  }
}

// ── Supporting widgets ──────────────────────────────────────────────────────

class _RpmReadout extends StatelessWidget {
  const _RpmReadout({required this.rpm, required this.isRunning});
  final double rpm;
  final bool   isRunning;

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Text(
          isRunning ? rpm.round().toString().padLeft(5, '0') : '00000',
          style: GoogleFonts.robotoMono(
            fontSize: 42,
            fontWeight: FontWeight.w700,
            color: AppTheme.onBackground,
            letterSpacing: 4,
          ),
        ),
        Text(
          'RPM',
          style: Theme.of(context).textTheme.labelSmall?.copyWith(
            letterSpacing: 4,
            color: AppTheme.onSurfaceDim,
          ),
        ),
      ],
    );
  }
}

class _StartStopButton extends StatefulWidget {
  const _StartStopButton({
    required this.isRunning,
    required this.onTap,
  });
  final bool      isRunning;
  final VoidCallback onTap;

  @override
  State<_StartStopButton> createState() => _StartStopButtonState();
}

class _StartStopButtonState extends State<_StartStopButton>
    with SingleTickerProviderStateMixin {
  late final AnimationController _pulse;

  @override
  void initState() {
    super.initState();
    _pulse = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1200),
    );
    if (widget.isRunning) _pulse.repeat(reverse: true);
  }

  @override
  void didUpdateWidget(_StartStopButton old) {
    super.didUpdateWidget(old);
    if (widget.isRunning && !old.isRunning) {
      _pulse.repeat(reverse: true);
    } else if (!widget.isRunning && old.isRunning) {
      _pulse.stop();
      _pulse.reset();
    }
  }

  @override
  void dispose() {
    _pulse.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: _pulse,
      builder: (context, _) {
        final glow = widget.isRunning ? _pulse.value : 0.0;
        return GestureDetector(
          onTap: widget.onTap,
          child: Container(
            width: 180,
            height: 56,
            decoration: BoxDecoration(
              color: widget.isRunning
                  ? AppTheme.accentRed
                  : AppTheme.accentGreen,
              borderRadius: BorderRadius.circular(28),
              boxShadow: [
                BoxShadow(
                  color: (widget.isRunning
                      ? AppTheme.accentRed
                      : AppTheme.accentGreen)
                      .withAlpha((glow * 120 + 40).round()),
                  blurRadius: 20 + glow * 10,
                  spreadRadius: glow * 4,
                ),
              ],
            ),
            alignment: Alignment.center,
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(
                  widget.isRunning
                      ? Icons.stop_rounded
                      : Icons.play_arrow_rounded,
                  color: Colors.white,
                  size: 28,
                ),
                const SizedBox(width: 8),
                Text(
                  widget.isRunning ? 'STOP ENGINE' : 'START ENGINE',
                  style: const TextStyle(
                    color: Colors.white,
                    fontWeight: FontWeight.w700,
                    fontSize: 16,
                    letterSpacing: 1.0,
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }
}
