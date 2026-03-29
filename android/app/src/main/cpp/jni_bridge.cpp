/**
 * JNI bridge between Kotlin [OboeAudioEngine] and the C++ [OboeEngine].
 *
 * Package: com.enginex.audio  (class OboeAudioEngine)
 *
 * The OboeEngine* is stored as a jlong on the Kotlin side (_nativeHandle).
 * All JNI functions recover the pointer via getEngine() before use.
 *
 * Thread safety:
 *   - nativeInit / nativeRelease called on the main thread at app lifecycle events.
 *   - nativeStart / nativeStop called from the Kotlin audio manager.
 *   - nativeSetParams called at ~60 Hz from Kotlin physics loop.
 *   - nativeSetProfile called on vehicle selection (infrequent).
 *   - nativeTriggerBackfire called on throttle-lift events.
 *
 * All cross-thread parameter writes go through std::atomic in OboeEngine/EngineSynthPro,
 * so none of these calls require additional locking.
 */

#include <jni.h>
#include <android/log.h>
#include "oboe_engine.h"

#define LOG_TAG "JNI_Bridge"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

using enginex::OboeEngine;
using enginex::kProMaxHarmonics;

// ── Handle helpers ────────────────────────────────────────────────────────────

static OboeEngine* getEngine(JNIEnv* env, jobject thiz) {
    jclass   clazz = env->GetObjectClass(thiz);
    jfieldID fid   = env->GetFieldID(clazz, "_nativeHandle", "J");
    if (!fid) { LOGE("_nativeHandle field not found"); return nullptr; }
    return reinterpret_cast<OboeEngine*>(env->GetLongField(thiz, fid));
}

static void setEngine(JNIEnv* env, jobject thiz, OboeEngine* e) {
    jclass   clazz = env->GetObjectClass(thiz);
    jfieldID fid   = env->GetFieldID(clazz, "_nativeHandle", "J");
    if (fid) env->SetLongField(thiz, fid, reinterpret_cast<jlong>(e));
}

/** Convert a jdoubleArray to a C float array. Returns element count. */
static int jdoubleArrayToFloat(JNIEnv* env, jdoubleArray arr,
                                float* out, int maxOut) {
    if (!arr) return 0;
    const jsize len = env->GetArrayLength(arr);
    jdouble* src    = env->GetDoubleArrayElements(arr, nullptr);
    const int count = (len < maxOut) ? static_cast<int>(len) : maxOut;
    for (int i = 0; i < count; ++i) out[i] = static_cast<float>(src[i]);
    env->ReleaseDoubleArrayElements(arr, src, JNI_ABORT);
    return count;
}

// ── JNI functions ─────────────────────────────────────────────────────────────

extern "C" {

// ── nativeInit ────────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeInit(
        JNIEnv*      env,
        jobject      thiz,
        jint         cylinders,
        jdouble      idleRpm,
        jdouble      revLimiterRpm,
        jdoubleArray harmonicWeightsJ,
        jdouble      noiseLevel,
        jdouble      combFeedback,
        jdouble      formantFreq0,
        jdouble      formantFreq1,
        jdouble      formantQ0,
        jdouble      formantQ1,
        jdouble      formantGain0,
        jdouble      formantGain1,
        jdouble      turboGain,
        jdouble      turboSpeedRatio,
        jint         turboBladeCount)
{
    auto* engine = new OboeEngine();
    setEngine(env, thiz, engine);

    float fw[kProMaxHarmonics]{};
    const int len = jdoubleArrayToFloat(env, harmonicWeightsJ, fw, kProMaxHarmonics);

    const bool ok = engine->init(
        static_cast<int>(cylinders),
        static_cast<float>(idleRpm),
        static_cast<float>(revLimiterRpm),
        fw, len,
        static_cast<float>(noiseLevel),
        static_cast<float>(combFeedback),
        static_cast<float>(formantFreq0),
        static_cast<float>(formantFreq1),
        static_cast<float>(formantQ0),
        static_cast<float>(formantQ1),
        static_cast<float>(formantGain0),
        static_cast<float>(formantGain1),
        static_cast<float>(turboGain),
        static_cast<float>(turboSpeedRatio),
        static_cast<int>(turboBladeCount)
    );
    LOGI("nativeInit: cyl=%d idle=%.0f ok=%d", cylinders, idleRpm, ok);
    return static_cast<jboolean>(ok);
}

// ── nativeStart ───────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeStart(
        JNIEnv* env, jobject thiz)
{
    OboeEngine* e = getEngine(env, thiz);
    if (!e) return JNI_FALSE;
    return static_cast<jboolean>(e->start());
}

// ── nativeStop ────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeStop(
        JNIEnv* env, jobject thiz)
{
    OboeEngine* e = getEngine(env, thiz);
    if (e) e->stop();
}

// ── nativeRelease ─────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeRelease(
        JNIEnv* env, jobject thiz)
{
    OboeEngine* e = getEngine(env, thiz);
    if (e) {
        e->release();
        delete e;
        setEngine(env, thiz, nullptr);
    }
}

// ── nativeSetParams ───────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeSetParams(
        JNIEnv* env, jobject thiz,
        jdouble rpm, jdouble throttle)
{
    OboeEngine* e = getEngine(env, thiz);
    if (e) e->setParams(static_cast<float>(rpm),
                        static_cast<float>(throttle));
}

// ── nativeSetProfile ──────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeSetProfile(
        JNIEnv*      env,
        jobject      thiz,
        jint         cylinders,
        jdoubleArray harmonicWeightsJ,
        jdouble      noiseLevel,
        jdouble      combFeedback,
        jdouble      formantFreq0,
        jdouble      formantFreq1,
        jdouble      formantQ0,
        jdouble      formantQ1,
        jdouble      formantGain0,
        jdouble      formantGain1,
        jdouble      turboGain,
        jdouble      turboSpeedRatio,
        jint         turboBladeCount,
        jdouble      intakeSampleGain,
        jdouble      exhaustSampleGain)
{
    OboeEngine* e = getEngine(env, thiz);
    if (!e) return;

    float fw[kProMaxHarmonics]{};
    const int len = jdoubleArrayToFloat(env, harmonicWeightsJ, fw, kProMaxHarmonics);

    e->setProfile(
        static_cast<int>(cylinders),
        fw, len,
        static_cast<float>(noiseLevel),
        static_cast<float>(combFeedback),
        static_cast<float>(formantFreq0),
        static_cast<float>(formantFreq1),
        static_cast<float>(formantQ0),
        static_cast<float>(formantQ1),
        static_cast<float>(formantGain0),
        static_cast<float>(formantGain1),
        static_cast<float>(turboGain),
        static_cast<float>(turboSpeedRatio),
        static_cast<int>(turboBladeCount),
        static_cast<float>(intakeSampleGain),
        static_cast<float>(exhaustSampleGain)
    );
}

// ── nativeSetGear ─────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeSetGear(
        JNIEnv* env, jobject thiz, jint gear)
{
    OboeEngine* e = getEngine(env, thiz);
    if (e) e->setGear(static_cast<int>(gear));
}

// ── nativeTriggerBackfire ─────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeTriggerBackfire(
        JNIEnv* env, jobject thiz)
{
    OboeEngine* e = getEngine(env, thiz);
    if (e) e->triggerBackfire();
}

// ── nativeIsRunning ───────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeIsRunning(
        JNIEnv* env, jobject thiz)
{
    OboeEngine* e = getEngine(env, thiz);
    return e ? static_cast<jboolean>(e->isRunning()) : JNI_FALSE;
}

} // extern "C"
