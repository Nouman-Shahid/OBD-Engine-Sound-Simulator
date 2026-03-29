package com.example.enginegamma.background

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.example.enginegamma.MainActivity

/**
 * Foreground service that keeps the engine audio alive when the app is backgrounded.
 *
 * Android requires a foreground service with a visible notification for
 * long-running audio playback. Without this, the OS will kill the process
 * within ~1 minute of the app going to background.
 *
 * Usage:
 *   Start:  startForegroundService(Intent(context, EngineAudioService::class.java).apply {
 *               action = ACTION_START
 *           })
 *   Stop:   startService(Intent(context, EngineAudioService::class.java).apply {
 *               action = ACTION_STOP
 *           })
 *
 * Note: The actual audio engine ([OboeAudioEngine]) is owned by [EngineChannelHandler]
 * in the main Flutter engine. This service simply keeps the process alive and
 * displays the notification. The audio thread continues running in the same process.
 */
class EngineAudioService : Service() {

    companion object {
        const val ACTION_START   = "com.enginex.action.START_AUDIO"
        const val ACTION_STOP    = "com.enginex.action.STOP_AUDIO"
        const val EXTRA_VEHICLE  = "com.enginex.extra.VEHICLE_NAME"
        const val NOTIFICATION_ID = 1001
        const val CHANNEL_ID      = "enginex_audio_channel"

        fun startIntent(context: Context, vehicleName: String = "Engine"): Intent =
            Intent(context, EngineAudioService::class.java).apply {
                action = ACTION_START
                putExtra(EXTRA_VEHICLE, vehicleName)
            }

        fun stopIntent(context: Context): Intent =
            Intent(context, EngineAudioService::class.java).apply {
                action = ACTION_STOP
            }
    }

    private var vehicleName = "Engine"

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> {
                vehicleName = intent.getStringExtra(EXTRA_VEHICLE) ?: "Engine"
                startForeground(NOTIFICATION_ID, buildNotification(vehicleName))
            }
            ACTION_STOP -> {
                stopForeground(true)
                stopSelf()
            }
        }
        // If killed, don't restart (audio would be in unknown state)
        return START_NOT_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? = null

    // ── Notification ──────────────────────────────────────────────────────────

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "Engine Audio",
                NotificationManager.IMPORTANCE_LOW   // no sound for this channel
            ).apply {
                description = "Engine sound playback in background"
                setShowBadge(false)
            }
            val nm = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            nm.createNotificationChannel(channel)
        }
    }

    private fun buildNotification(name: String): Notification {
        val tapIntent = Intent(this, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_SINGLE_TOP
        }
        val tapPending = PendingIntent.getActivity(
            this, 0, tapIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val stopPending = PendingIntent.getService(
            this, 1,
            stopIntent(this),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.ic_media_play)
            .setContentTitle("EngineX — $name")
            .setContentText("Engine running in background")
            .setOngoing(true)
            .setSilent(true)
            .setContentIntent(tapPending)
            .addAction(android.R.drawable.ic_media_pause, "Stop", stopPending)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
    }
}
