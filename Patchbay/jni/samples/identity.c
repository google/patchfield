#include "identity.h"

#include "audio_module.h"

#include <stddef.h>
#include <string.h>

static void process_func(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  memcpy(output_buffer, input_buffer,
      buffer_frames * input_channels * sizeof(float));
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_IdentityModule_createModule
(JNIEnv *env, jobject obj, jint version, jint token, jint index) {
  return (jlong) am_create(version, token, index, process_func, NULL);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_IdentityModule_release
(JNIEnv *env, jobject obj, jlong p) {
  am_release((audio_module_runner *) p);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_IdentityModule_hasTimedOut
(JNIEnv *env, jobject obj, jlong p) {
  return am_has_timed_out((audio_module_runner *) p);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_IdentityModule_getProtocolVersion
(JNIEnv *env, jobject obj) {
  return PATCHBAY_PROTOCOL_VERSION;
}
