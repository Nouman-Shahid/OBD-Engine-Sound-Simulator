/**
 * JNI bridge between Kotlin [OboeAudioEngine] and the C++ [OboeEngine].
 *
 * Each JNI function maps 1:1 to a private external fun in OboeAudioEngine.kt.
 * The OboeEngine instance is stored as a jlong handle (pointer-as-int) on
 * the Kotlin side — this is the idiomatic Android NDK pattern.
 *
 * Package: com.enginex.audio  ↔  class OboeAudioEngine
 */

#include <jni.h>
#include <android/log.h>
#include "oboe_engine.h"

#define LOG_TAG "JNI_Bridge"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using enginex::OboeEngine;

// ── Helper: recover OboeEngine* from the hidden Kotlin field ─────────────────

static OboeEngine* getEngine(JNIEnv* env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(clazz, "_nativeHandle", "J");
    if (!fid) {
        LOGE("_nativeHandle field not found");
        return nullptr;
    }
    return reinterpret_cast<OboeEngine*>(env->GetLongField(thiz, fid));
}

static void setEngine(JNIEnv* env, jobject thiz, OboeEngine* engine) {
    jclass clazz = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(clazz, "_nativeHandle", "J");
    if (fid) env->SetLongField(thiz, fid, reinterpret_cast<jlong>(engine));
}

// ── JNI functions ─────────────────────────────────────────────────────────────

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeInit(
        JNIEnv*      env,
        jobject      thiz,
        jint         cylinders,
        jdouble      idleRpm,
        jdouble      revLimiterRpm,
        jdoubleArray harmonicWeightsJ,
        jdouble      noiseLevel)
{
    // Create new engine instance and attach to Kotlin object.
    auto* engine = new OboeEngine();
    setEngine(env, thiz, engine);

    // Convert jdoubleArray → float[].
    jsize   len     = env->GetArrayLength(harmonicWeightsJ);
    jdouble* src    = env->GetDoubleArrayElements(harmonicWeightsJ, nullptr);
    float floatWeights[enginex::kMaxHarmonics]{};
    for (int i = 0; i < len && i < enginex::kMaxHarmonics; ++i) {
        floatWeights[i] = static_cast<float>(src[i]);
    }
    env->ReleaseDoubleArrayElements(harmonicWeightsJ, src, JNI_ABORT);

    bool ok = engine->init(
        static_cast<int>(cylinders),
        static_cast<float>(idleRpm),
        static_cast<float>(revLimiterRpm),
        floatWeights,
        static_cast<int>(len),
        static_cast<float>(noiseLevel)
    );
    return static_cast<jboolean>(ok);
}

JNIEXPORT jboolean JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeStart(
        JNIEnv* env, jobject thiz)
{
    OboeEngine* e = getEngine(env, thiz);
    if (!e) return JNI_FALSE;
    return static_cast<jboolean>(e->start());
}

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeStop(
        JNIEnv* env, jobject thiz)
{
    OboeEngine* e = getEngine(env, thiz);
    if (e) e->stop();
}

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

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeSetParams(
        JNIEnv* env, jobject thiz,
        jdouble rpm, jdouble throttle)
{
    OboeEngine* e = getEngine(env, thiz);
    if (e) e->setParams(static_cast<float>(rpm),
                        static_cast<float>(throttle));
}

JNIEXPORT void JNICALL
Java_com_enginex_audio_OboeAudioEngine_nativeSetProfile(
        JNIEnv*      env,
        jobject      thiz,
        jint         cylinders,
        jdoubleArray harmonicWeightsJ,
        jdouble      noiseLevel)
{
    OboeEngine* e = getEngine(env, thiz);
    if (!e) return;

    jsize   len  = env->GetArrayLength(harmonicWeightsJ);
    jdouble* src = env->GetDoubleArrayElements(harmonicWeightsJ, nullptr);
    float fw[enginex::kMaxHarmonics]{};
    for (int i = 0; i < len && i < enginex::kMaxHarmonics; ++i) {
        fw[i] = static_cast<float>(src[i]);
    }
    env->ReleaseDoubleArrayElements(harmonicWeightsJ, src, JNI_ABORT);

    e->setProfile(static_cast<int>(cylinders), fw,
                  static_cast<int>(len),
                  static_cast<float>(noiseLevel));
}

} // extern "C"
