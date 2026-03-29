package com.example.enginegamma

import android.content.Intent
import android.os.Bundle
import com.example.enginegamma.background.EngineAudioService
import com.example.enginegamma.engine.EngineChannelHandler
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine

/**
 * Main activity — owns the [EngineChannelHandler] that wires
 * Flutter ↔ Kotlin ↔ C++ (Oboe) together.
 *
 * Lifecycle:
 *  onCreate    → [EngineChannelHandler] is created (Oboe stream opened but not started)
 *  onDestroy   → [EngineChannelHandler.release] (Oboe stream closed, resources freed)
 *  onPause     → background service started to keep audio alive if engine is running
 *  onResume    → background service stopped (app is foregrounded again)
 */
class MainActivity : FlutterActivity() {

    private var channelHandler: EngineChannelHandler? = null

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        channelHandler = EngineChannelHandler(this, flutterEngine)
    }

    override fun onResume() {
        super.onResume()
        // Stop the background foreground service — app is visible again
        startService(EngineAudioService.stopIntent(this))
    }

    override fun onPause() {
        super.onPause()
        // If the engine is running, keep it alive in the background via a
        // foreground service so Android doesn't kill the process
        // The actual audio runs in the same process — service just prevents kill.
        if (channelHandler != null) {
            val intent = EngineAudioService.startIntent(this, "Engine Running")
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
                startForegroundService(intent)
            } else {
                startService(intent)
            }
        }
    }

    override fun onDestroy() {
        channelHandler?.release()
        channelHandler = null
        startService(EngineAudioService.stopIntent(this))
        super.onDestroy()
    }
}
