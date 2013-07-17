#include "javamodule.h"

#include "audio_module.h"
#include "utils/buffer_size_adapter.h"
#include "internal/simple_barrier.h"

#include <stddef.h>
#include <string.h>

typedef struct {
  buffer_size_adapter *bsa;
  simple_barrier_t wake;
  simple_barrier_t ready;
  float *input_buffer;
  float *output_buffer;
  int done;
} jmodule;

static void process_jm(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  jmodule *jm = (jmodule *) context;
  jm->input_buffer = input_buffer;
  jm->output_buffer = output_buffer;
  sb_wake(&jm->wake);
  sb_wait_and_clear(&jm->ready, NULL);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchbay_modules_JavaModule_getProtocolVersion
(JNIEnv *env, jobject obj) {
  return PATCHBAY_PROTOCOL_VERSION;
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchbay_modules_JavaModule_hasTimedOut
(JNIEnv *env, jobject obj, jlong p) {
  jmodule *jm = (jmodule *) p;
  return am_has_timed_out(bsa_get_runner(jm->bsa));
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchbay_modules_JavaModule_configure
(JNIEnv *env, jobject obj, jint version, jint token, jint index, jint
 host_buffer_size, jint user_buffer_size, jint input_channels, jint
 output_channels) {
  jmodule *jm = malloc(sizeof(jmodule));
  if (jm) {
    sb_clobber(&jm->wake);
    sb_clobber(&jm->ready);
    jm->input_buffer = NULL;
    jm->output_buffer = NULL;
    jm->done = 0;
    jm->bsa = bsa_create(version, token, index, host_buffer_size,
        user_buffer_size, input_channels, output_channels, process_jm, jm);
    if (!jm->bsa) {
      free(jm);
      jm = NULL;
    }
  }
  return jm;
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_modules_JavaModule_release
(JNIEnv *env, jobject obj, jlong p) {
  jmodule *jm = (jmodule *) p;
  bsa_release(jm->bsa);
  free(jm);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_modules_JavaModule_fillInputBuffer
(JNIEnv *env, jobject obj, jlong p, jfloatArray buffer) {
  jmodule *jm = (jmodule *) p;
  sb_wait_and_clear(&jm->wake, NULL);
  if (!jm->done) {
    int n = (*env)->GetArrayLength(env, buffer);
    float *b = (*env)->GetFloatArrayElements(env, buffer, NULL);
    memcpy(b, jm->input_buffer, n * sizeof(float));
    (*env)->ReleaseFloatArrayElements(env, buffer, b, 0);
  }
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_modules_JavaModule_sendOutputBuffer
(JNIEnv *env, jobject obj, jlong p, jfloatArray buffer) {
  jmodule *jm = (jmodule *) p;
  if (!jm->done) {
    int n = (*env)->GetArrayLength(env, buffer);
    float *b = (*env)->GetFloatArrayElements(env, buffer, NULL);
    memcpy(jm->output_buffer, b, n * sizeof(float));
    (*env)->ReleaseFloatArrayElements(env, buffer, b, 0);
    sb_wake(&jm->ready);
  }
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_modules_JavaModule_signalThread
(JNIEnv *env, jobject obj, jlong p) {
  jmodule *jm = (jmodule *) p;
  jm->done = 1;
  sb_wake(&jm->wake);
}

