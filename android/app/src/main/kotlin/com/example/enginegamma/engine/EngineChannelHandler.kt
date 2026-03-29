package com.example.enginegamma.engine

import android.content.Context
import android.os.Handler
import android.os.Looper
import com.enginex.audio.OboeAudioEngine
import com.example.enginegamma.audio.AudioFocusManager
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel

/**
 * Bridges the Flutter platform channels to the native audio engine.
 *
 * Method channel  : com.enginex/engine_method   (Flutter → Kotlin)
 * Event channel   : com.enginex/engine_event    (Kotlin → Flutter, ~60 Hz)
 * OBD Method ch.  : com.enginex/obd_method      (Flutter → Kotlin)
 * OBD Event ch.   : com.enginex/obd_event       (Kotlin → Flutter)
 *
 * Architecture:
 *   Flutter UI → MethodChannel → EngineChannelHandler
 *                                   ↓ physics loop (60 Hz on main thread)
 *                              EnginePhysics → OboeAudioEngine (C++ JNI)
 *                                   ↓ event stream
 *                              EventChannel → Flutter UI
 */
class EngineChannelHandler(
    private val context: Context,
    flutterEngine: FlutterEngine
) {
    companion object {
        const val METHOD_CHANNEL = "com.enginex/engine_method"
        const val EVENT_CHANNEL  = "com.enginex/engine_event"
        const val OBD_METHOD_CHANNEL = "com.enginex/obd_method"
        const val OBD_EVENT_CHANNEL  = "com.enginex/obd_event"

        private const val TICK_MS = 16L   // ~60 Hz
    }

    // ── Core components ───────────────────────────────────────────────────────

    private val audioEngine    = OboeAudioEngine()
    private val physics        = EnginePhysics()
    private val audioFocus     = AudioFocusManager(context)
    private var obdService: com.example.enginegamma.obd.OBDService? = null

    // ── Flutter channels ──────────────────────────────────────────────────────

    private val methodChannel = MethodChannel(
        flutterEngine.dartExecutor.binaryMessenger, METHOD_CHANNEL)
    private val eventChannel  = EventChannel(
        flutterEngine.dartExecutor.binaryMessenger, EVENT_CHANNEL)
    private var eventSink: EventChannel.EventSink? = null

    private val obdMethodChannel = MethodChannel(
        flutterEngine.dartExecutor.binaryMessenger, OBD_METHOD_CHANNEL)
    private val obdEventChannel  = EventChannel(
        flutterEngine.dartExecutor.binaryMessenger, OBD_EVENT_CHANNEL)
    private var obdEventSink: EventChannel.EventSink? = null

    // ── Physics update loop ───────────────────────────────────────────────────

    private val handler = Handler(Looper.getMainLooper())
    private var isLooping = false

    private val tickRunnable = object : Runnable {
        override fun run() {
            if (!isLooping) return
            tick()
            handler.postDelayed(this, TICK_MS)
        }
    }

    // ── State ─────────────────────────────────────────────────────────────────

    private var currentVehicleId = "sport_4cyl"
    private var isRunning        = false

    // ── Init ──────────────────────────────────────────────────────────────────

    init {
        setupMethodChannel()
        setupEventChannel()
        setupObdChannels()
        initAudioEngineWithProfile(VehicleProfiles.sport4)
    }

    private fun initAudioEngineWithProfile(p: VehicleProfiles.VehicleProfile) {
        physics.setProfile(p)
        audioEngine.init(
            cylinders       = p.cylinders,
            idleRpm         = p.idleRpm,
            revLimiterRpm   = p.revLimiterRpm,
            harmonicWeights = p.harmonicWeights,
            noiseLevel      = p.noiseLevel,
            combFeedback    = p.combFeedback,
            formantFreq0    = p.formantFreq0,
            formantFreq1    = p.formantFreq1,
            formantQ0       = p.formantQ0,
            formantQ1       = p.formantQ1,
            formantGain0    = p.formantGain0,
            formantGain1    = p.formantGain1,
            turboGain       = p.turboGain,
            turboSpeedRatio = p.turboSpeedRatio,
            turboBladeCount = p.turboBladeCount
        )
    }

    // ── Method channel setup ──────────────────────────────────────────────────

    private fun setupMethodChannel() {
        methodChannel.setMethodCallHandler { call, result ->
            when (call.method) {
                "startEngine"     -> result.success(handleStart())
                "stopEngine"      -> { handleStop(); result.success(null) }
                "setThrottle"     -> {
                    val t = (call.argument<Double>("throttle") ?: 0.0)
                    physics.setThrottle(t)
                    result.success(null)
                }
                "setGear"         -> {
                    val g = (call.argument<Int>("gear") ?: 0)
                    physics.setGear(g)
                    result.success(null)
                }
                "setVehicle"      -> {
                    val id = call.argument<String>("vehicleId") ?: "sport_4cyl"
                    handleSetVehicle(id)
                    result.success(null)
                }
                "isEngineRunning" -> result.success(isRunning)
                "triggerBackfire" -> { audioEngine.triggerBackfire(); result.success(null) }
                else              -> result.notImplemented()
            }
        }
    }

    // ── Event channel setup ───────────────────────────────────────────────────

    private fun setupEventChannel() {
        eventChannel.setStreamHandler(object : EventChannel.StreamHandler {
            override fun onListen(arguments: Any?, sink: EventChannel.EventSink) {
                eventSink = sink
            }
            override fun onCancel(arguments: Any?) {
                eventSink = null
            }
        })
    }

    // ── OBD channel setup ─────────────────────────────────────────────────────

    private fun setupObdChannels() {
        obdMethodChannel.setMethodCallHandler { call, result ->
            handleObdMethod(call, result)
        }
        obdEventChannel.setStreamHandler(object : EventChannel.StreamHandler {
            override fun onListen(arguments: Any?, sink: EventChannel.EventSink) {
                obdEventSink = sink
            }
            override fun onCancel(arguments: Any?) {
                obdEventSink = null
            }
        })
    }

    private fun handleObdMethod(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "scanDevices" -> {
                val devices = getObdService().scanPairedDevices()
                result.success(devices)
            }
            "connect" -> {
                val addr = call.argument<String>("address") ?: run {
                    result.error("BAD_ARG", "address required", null); return
                }
                getObdService().connect(addr, onConnected = {
                    handler.post { obdEventSink?.success(mapOf("event" to "connected", "address" to addr)) }
                }, onData = { rpm, throttle ->
                    physics.setObdData(rpm, throttle)
                    handler.post { obdEventSink?.success(mapOf("event" to "data", "rpm" to rpm, "throttle" to throttle)) }
                }, onDisconnected = {
                    physics.clearObdData()
                    handler.post { obdEventSink?.success(mapOf("event" to "disconnected")) }
                })
                result.success(null)
            }
            "disconnect" -> {
                obdService?.disconnect()
                physics.clearObdData()
                result.success(null)
            }
            "isConnected" -> result.success(obdService?.isConnected() ?: false)
            else          -> result.notImplemented()
        }
    }

    private fun getObdService(): com.example.enginegamma.obd.OBDService {
        if (obdService == null) {
            obdService = com.example.enginegamma.obd.OBDService(context)
        }
        return obdService!!
    }

    // ── Engine start/stop ─────────────────────────────────────────────────────

    private fun handleStart(): Boolean {
        if (isRunning) return true

        // Request audio focus before starting
        val hasFocus = audioFocus.requestFocus(
            onLoss = { handleStop() },
            onLossTransient = { audioEngine.setParams(0.0, 0.0) }
        )
        if (!hasFocus) return false

        physics.start()
        val ok = audioEngine.start()
        if (ok) {
            isRunning = true
            startLoop()
        }
        return ok
    }

    private fun handleStop() {
        isRunning = false
        stopLoop()
        audioEngine.stop()
        physics.stop()
        audioFocus.abandonFocus()
        // Push a final stopped state
        pushState(rpm = 0.0, throttle = 0.0, gear = 0, revLimiting = false, layer = "idle")
    }

    private fun handleSetVehicle(id: String) {
        currentVehicleId = id
        val profile = VehicleProfiles.findById(id)
        physics.setProfile(profile)
        audioEngine.setProfile(
            cylinders       = profile.cylinders,
            harmonicWeights = profile.harmonicWeights,
            noiseLevel      = profile.noiseLevel,
            combFeedback    = profile.combFeedback,
            formantFreq0    = profile.formantFreq0,
            formantFreq1    = profile.formantFreq1,
            formantQ0       = profile.formantQ0,
            formantQ1       = profile.formantQ1,
            formantGain0    = profile.formantGain0,
            formantGain1    = profile.formantGain1,
            turboGain       = profile.turboGain,
            turboSpeedRatio = profile.turboSpeedRatio,
            turboBladeCount = profile.turboBladeCount
        )
    }

    // ── 60 Hz physics + DSP loop ──────────────────────────────────────────────

    private fun startLoop() {
        if (isLooping) return
        isLooping = true
        handler.post(tickRunnable)
    }

    private fun stopLoop() {
        isLooping = false
        handler.removeCallbacks(tickRunnable)
    }

    private fun tick() {
        val (rpm, throttle) = physics.update(TICK_MS.toDouble())

        // Forward to DSP
        audioEngine.setParams(rpm, throttle)

        // Auto backfire on throttle lift
        if (physics.checkBackfire()) audioEngine.triggerBackfire()

        // Push event to Flutter
        val currentGear = getCurrentGear()
        pushState(
            rpm          = rpm,
            throttle     = throttle,
            gear         = currentGear,
            revLimiting  = physics.isRevLimiting,
            layer        = physics.getActiveLayer()
        )
    }

    private fun getCurrentGear(): Int {
        // Read back the gear from the physics — keep a local field
        return _currentGear
    }

    private var _currentGear = 0

    private fun pushState(
        rpm: Double, throttle: Double, gear: Int,
        revLimiting: Boolean, layer: String
    ) {
        val engineState = when {
            !isRunning   -> "stopped"
            revLimiting  -> "rev_limiting"
            else         -> "running"
        }
        eventSink?.success(mapOf(
            "rpm"         to rpm,
            "throttle"    to throttle,
            "gear"        to gear,
            "engineState" to engineState,
            "activeLayer" to layer
        ))
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────

    fun release() {
        handleStop()
        audioEngine.release()
        obdService?.disconnect()
        methodChannel.setMethodCallHandler(null)
        eventChannel.setStreamHandler(null)
        obdMethodChannel.setMethodCallHandler(null)
        obdEventChannel.setStreamHandler(null)
    }
}
