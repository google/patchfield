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

#include "message.h"

#include "audio_module.h"
#include "tinyosc/src/tinyosc.h"

#include <android/log.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "message", __VA_ARGS__)
#define LOGW(...) \
  __android_log_print(ANDROID_LOG_WARN, "message", __VA_ARGS__)

typedef struct {
  void *handle;
} message_data;

static void process_func(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  message_data *data = (message_data *) context;
  am_message message = { 0, NULL };
  char s[256];
  while (!am_next_message(data->handle, &message)) {
    osc_message_to_string(s, 256, (osc_packet *) &message);
    LOGI("%s", s);
  }
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchfield_messagesample_MessageModule_configureNativeComponents
(JNIEnv *env, jobject obj, jlong handle) {
  message_data *data = malloc(sizeof(message_data));
  if (data) {
    data->handle = (void *) handle;
    am_configure((void *) handle, process_func, data);
  }
  return (jlong) data;
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_messagesample_MessageModule_release
(JNIEnv *env, jobject obj, jlong p) {
  message_data *data = (message_data *) p;
  free(data);
}
