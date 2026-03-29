package com.example.enginegamma.audio

import android.content.Context
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build

/**
 * Manages Android audio focus for the engine audio stream.
 *
 * Audio focus ensures the engine sound:
 *  - Ducks or pauses when a phone call arrives
 *  - Pauses when another app claims full focus (e.g. music player)
 *  - Resumes when focus is returned
 *
 * Uses the modern [AudioFocusRequest] API on Android O (API 26)+
 * with a legacy fallback for older devices.
 */
class AudioFocusManager(private val context: Context) {

    private val audioManager =
        context.getSystemService(Context.AUDIO_SERVICE) as AudioManager

    private var focusRequest: AudioFocusRequest? = null   // API 26+
    private var legacyListener: AudioManager.OnAudioFocusChangeListener? = null

    /**
     * Request audio focus for media playback.
     *
     * @param onLoss           Called when focus is lost permanently (stop engine).
     * @param onLossTransient  Called when focus is temporarily lost (mute quickly).
     * @return true if focus was granted.
     */
    fun requestFocus(
        onLoss: () -> Unit,
        onLossTransient: () -> Unit
    ): Boolean {
        val listener = AudioManager.OnAudioFocusChangeListener { change ->
            when (change) {
                AudioManager.AUDIOFOCUS_LOSS              -> onLoss()
                AudioManager.AUDIOFOCUS_LOSS_TRANSIENT    -> onLossTransient()
                AudioManager.AUDIOFOCUS_GAIN              -> { /* resume handled by caller */ }
                AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> { /* lower volume only */ }
            }
        }

        val result: Int

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val req = AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                .setAudioAttributes(
                    AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                        .build()
                )
                .setAcceptsDelayedFocusGain(false)
                .setOnAudioFocusChangeListener(listener)
                .build()
            focusRequest = req
            result = audioManager.requestAudioFocus(req)
        } else {
            @Suppress("DEPRECATION")
            result = audioManager.requestAudioFocus(
                listener,
                AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN
            )
            legacyListener = listener
        }

        return result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED
    }

    /**
     * Release audio focus when the engine is stopped.
     */
    fun abandonFocus() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            focusRequest?.let { audioManager.abandonAudioFocusRequest(it) }
            focusRequest = null
        } else {
            @Suppress("DEPRECATION")
            legacyListener?.let { audioManager.abandonAudioFocus(it) }
            legacyListener = null
        }
    }
}
