import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class AppTheme {
  AppTheme._();

  // ── Brand palette ─────────────────────────────────────────────────────────
  static const Color background     = Color(0xFF0D0D0F);
  static const Color surface        = Color(0xFF161618);
  static const Color surfaceVariant = Color(0xFF1E1E22);
  static const Color border         = Color(0xFF2A2A30);

  static const Color accentOrange   = Color(0xFFFF6B00);
  static const Color accentRed      = Color(0xFFE63946);
  static const Color accentGreen    = Color(0xFF2DC653);
  static const Color accentBlue     = Color(0xFF4CC9F0);

  static const Color onBackground   = Color(0xFFF0F0F0);
  static const Color onSurface      = Color(0xFFCCCCCC);
  static const Color onSurfaceDim   = Color(0xFF888899);

  // ── RPM zone colours ──────────────────────────────────────────────────────
  static const Color rpmIdle   = Color(0xFF4CC9F0);
  static const Color rpmLow    = Color(0xFF2DC653);
  static const Color rpmMid    = Color(0xFFFFBE0B);
  static const Color rpmHigh   = Color(0xFFFF6B00);
  static const Color rpmDanger = Color(0xFFE63946);

  // ── Theme ─────────────────────────────────────────────────────────────────
  static ThemeData get dark {
    final base = ThemeData.dark(useMaterial3: true);
    return base.copyWith(
      scaffoldBackgroundColor: background,
      colorScheme: const ColorScheme.dark(
        primary:       accentOrange,
        secondary:     accentBlue,
        surface:       surface,
        onPrimary:     Colors.white,
        onSecondary:   Colors.white,
        onSurface:     onSurface,
        error:         accentRed,
      ),
      textTheme: GoogleFonts.rajdhaniTextTheme(base.textTheme).copyWith(
        displayLarge: GoogleFonts.rajdhani(
          fontSize: 72, fontWeight: FontWeight.w700, color: onBackground,
          letterSpacing: -1,
        ),
        headlineMedium: GoogleFonts.rajdhani(
          fontSize: 28, fontWeight: FontWeight.w600, color: onBackground,
        ),
        titleLarge: GoogleFonts.rajdhani(
          fontSize: 20, fontWeight: FontWeight.w600, color: onBackground,
          letterSpacing: 0.5,
        ),
        bodyMedium: GoogleFonts.rajdhani(
          fontSize: 15, color: onSurface,
        ),
        labelSmall: GoogleFonts.robotoMono(
          fontSize: 11, color: onSurfaceDim, letterSpacing: 1.2,
        ),
      ),
      cardTheme: CardThemeData(
        color: surfaceVariant,
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
          side: const BorderSide(color: border, width: 1),
        ),
      ),
      sliderTheme: SliderThemeData(
        activeTrackColor:   accentOrange,
        inactiveTrackColor: border,
        thumbColor:         accentOrange,
        overlayColor:       accentOrange.withAlpha(40),
        trackHeight:        6,
        thumbShape:         const RoundSliderThumbShape(enabledThumbRadius: 10),
      ),
    );
  }
}
