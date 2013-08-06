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

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchbay_modules_IdentityModule_configure
(JNIEnv *env, jobject obj, jlong p) {
  am_configure((void *) p, process_func, NULL);
  return 1;
}
