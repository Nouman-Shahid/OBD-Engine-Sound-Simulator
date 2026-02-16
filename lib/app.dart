import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:enginex/core/theme/app_theme.dart';
import 'package:enginex/features/engine/presentation/screens/vehicle_selector_screen.dart';

class EngineXApp extends ConsumerWidget {
  const EngineXApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return MaterialApp(
      title: 'EngineX',
      debugShowCheckedModeBanner: false,
      theme: AppTheme.dark,
      home: const VehicleSelectorScreen(),
    );
  }
}
