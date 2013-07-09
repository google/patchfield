#include "pcmsource.h"

#include "audio_module.h"

typedef struct {
  audio_module_runner *handle;
  float *buffer;
  int index;
  int size;
} pcm_source;

static void process_func(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  pcm_source *fs = (pcm_source *) context;
  int n = (fs->size - fs->index) / output_channels;
  if (n > buffer_frames) {
    n = buffer_frames;
  }
  int i, j;
  for (i = 0; i < output_channels; ++i) {
    for (j = 0; j < n; ++j) {
      output_buffer[j] = (fs->buffer + fs->index)[i + j * output_channels];
    }
    for (; j < buffer_frames; ++j) {
      output_buffer[j] = 0;
    }
    output_buffer += buffer_frames;
  }
  fs->index += n * output_channels;
  if (fs->index >= fs->size) {
    fs->index = 0;
  }
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchbay_source_PcmSource_createSource
(JNIEnv *env, jobject obj, jint token, jint index, jobject buffer) {
  pcm_source *data = malloc(sizeof(pcm_source));
  data->handle = am_create(token, index, process_func, data);
  data->buffer = (*env)->GetDirectBufferAddress(env, buffer);
  data->size =
    (*env)->GetDirectBufferCapacity(env, buffer) / sizeof(float);
  data->index = 0;
  return (jlong) data;
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_source_PcmSource_release
(JNIEnv *env, jobject obj, jlong p) {
  pcm_source *data = (pcm_source *) p;
  am_release(data->handle);
  free(data);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchbay_source_PcmSource_hasTimedOut
(JNIEnv *env, jobject obj, jlong p) {
  pcm_source *data = (pcm_source *) p;
  return am_has_timed_out(data->handle);
}
