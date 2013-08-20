/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include "pcmsource.h"

#include "audio_module.h"

#include <stddef.h>
#include <stdlib.h>

typedef struct {
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
Java_com_noisepages_nettoyeur_patchfield_source_PcmSource_createSource
(JNIEnv *env, jobject obj, jlong p, jobject buffer) {
  pcm_source *data = malloc(sizeof(pcm_source));
  if (data) {
    am_configure((void *) p, process_func, data);
    data->buffer = (*env)->GetDirectBufferAddress(env, buffer);
    data->size =
      (*env)->GetDirectBufferCapacity(env, buffer) / sizeof(float);
    data->index = 0;
  }
  return (jlong) data;
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_source_PcmSource_release
(JNIEnv *env, jobject obj, jlong p) {
  pcm_source *data = (pcm_source *) p;
  free(data);
}
