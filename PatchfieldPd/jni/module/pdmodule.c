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

#include "pdmodule.h"

#include "../libpd/jni/z_jni_native_hooks.h"
#include "audio_module.h"
#include "utils/buffer_size_adapter.h"

#include <stddef.h>

static void process_pd(void *context, int sample_rate, int buffer_frames,
    int input_channels, const float *input_buffer,
    int output_channels, float *output_buffer) {
  libpd_sync_process_raw(input_buffer, output_buffer);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_pd_PdModule_pdInitAudio
(JNIEnv *env, jobject obj, jint input_channels, jint output_channels,
 jint sample_rate) {
  libpd_sync_init_audio(input_channels, output_channels, sample_rate);
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchfield_pd_PdModule_configureModule
(JNIEnv *env, jobject obj, jlong handle,
 jint host_buffer_size, jint user_buffer_size,
 jint input_channels, jint output_channels) {
  return (jlong) bsa_create((void *) handle,
      host_buffer_size, user_buffer_size,
      input_channels, output_channels,
      process_pd, NULL);     
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_pd_PdModule_release
(JNIEnv *env, jobject obj, jlong p) {
  bsa_release((buffer_size_adapter *) p);
}
