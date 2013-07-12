#include "lowpass.h"

#include "audio_module.h"

#include <stddef.h>
#include <string.h>

#define RANGE 1000000
#define MAX_CHANNELS 8

typedef struct {
  audio_module_runner *handle;
  int a;
  float y[MAX_CHANNELS];
} lowpass_data;

static void process_func(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  lowpass_data *data = (lowpass_data *) context;
  float alpha = (float) __sync_fetch_and_or(&data->a, 0) / RANGE;
  int i, j;
  for (i = 0; i < input_channels; ++i) {
    float y = data->y[i];
    for (j = 0; j < buffer_frames; ++j) {
      float x = alpha * input_buffer[j] + (1 - alpha) * y;
      output_buffer[j] = y = x;
    }
    data->y[i] = y;
    input_buffer += buffer_frames;
    output_buffer += buffer_frames;
  }
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_LowpassModule_getMaxChannels
(JNIEnv *env, jclass cls) {
  return MAX_CHANNELS;
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_LowpassModule_setParameter
(JNIEnv *env, jobject obj, jlong p, jdouble alpha) {
  lowpass_data *data = (lowpass_data *) p;
  __sync_bool_compare_and_swap(&data->a, data->a, (int) (RANGE * alpha));
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_LowpassModule_createModule
(JNIEnv *env, jobject obj, jint version, jint token, jint index, jint channels) {
  lowpass_data *data = malloc(sizeof(lowpass_data));
  if (data) {
    data->handle = am_create(version, token, index, process_func, data);
    if (data->handle != NULL) {
      data->a = RANGE;
      int i;
      for (i = 0; i < MAX_CHANNELS; ++i) {
        data->y[i] = 0;
      }
    } else {
      free(data);
      data = NULL;
    }
  }
  return (jlong) data;
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_LowpassModule_release
(JNIEnv *env, jobject obj, jlong p) {
  lowpass_data *data = (lowpass_data *) p;
  am_release(data->handle);
  free(data);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_LowpassModule_hasTimedOut
(JNIEnv *env, jobject obj, jlong p) {
  lowpass_data *data = (lowpass_data *) p;
  return am_has_timed_out(data->handle);
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchbay_samples_LowpassModule_getProtocolVersion
(JNIEnv *env, jobject obj) {
  return PATCHBAY_PROTOCOL_VERSION;
}
