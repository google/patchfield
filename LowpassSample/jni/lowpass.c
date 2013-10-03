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

#include "lowpass.h"

#include "audio_module.h"
#include "tinyosc.h"

#include <android/log.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "lowpass", __VA_ARGS__)
#define LOGW(...) \
  __android_log_print(ANDROID_LOG_WARN, "lowpass", __VA_ARGS__)

#define RANGE 1000000

typedef struct {
  int alpha;  // RC lowpass filter coefficient, between 0 and RANGE.
  float *y;   // Filter values for each channel.
  void *handle;
} lowpass_data;

static void process_func(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  lowpass_data *data = (lowpass_data *) context;
  am_message msg = { 0, NULL };
  while (!am_next_message(data->handle, &msg)) {
    LOGI("OSC message: %d, %s, %p", msg.size, msg.data, msg.data);
  }
  float alpha = (float) __sync_fetch_and_or(&data->alpha, 0) / RANGE;
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

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchfield_lowpass_LowpassModule_configureNativeComponents
(JNIEnv *env, jobject obj, jlong handle, jint channels) {
  lowpass_data *data = malloc(sizeof(lowpass_data));
  if (data) {
    data->y = malloc(channels * sizeof(float));
    if (data->y) {
      data->handle = (void *) handle;
      data->alpha = RANGE;
      int i;
      for (i = 0; i < channels; ++i) {
        data->y[i] = 0;
      }
      am_configure(data->handle, process_func, data);
    } else {
      free(data);
      data = NULL;
    }
  }
  return (jlong) data;
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_lowpass_LowpassModule_release
(JNIEnv *env, jobject obj, jlong p) {
  lowpass_data *data = (lowpass_data *) p;
  free(data->y);
  free(data);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_lowpass_LowpassModule_setParameter
(JNIEnv *env, jobject obj, jlong p, jdouble alpha) {
  lowpass_data *data = (lowpass_data *) p;
  __sync_bool_compare_and_swap(&data->alpha, data->alpha, (int) (RANGE * alpha));
}
