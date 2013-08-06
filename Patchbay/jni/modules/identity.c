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

/*
 * Native component of the identity audio module. The process function simply
 * copies the input buffer to the output buffer.
 */

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
