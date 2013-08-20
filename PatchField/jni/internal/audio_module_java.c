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

#include "audio_module_java.h"

#include "audio_module_internal.h"

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchfield_AudioModule_getProtocolVersion
(JNIEnv *env, jclass cls) {
  return PATCHFIELD_PROTOCOL_VERSION;
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchfield_AudioModule_createRunner
(JNIEnv *env, jobject obj, jint version, jint token, jint index) {
  return (jlong) ami_create(version, token, index);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchfield_AudioModule_release
(JNIEnv *env, jobject obj, jlong p) {
  audio_module_runner *amr = (audio_module_runner *) p;
  ami_release(amr);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchfield_AudioModule_hasTimedOut
(JNIEnv *env, jobject obj, jlong p) {
  audio_module_runner *amr = (audio_module_runner *) p;
  return ami_has_timed_out(amr);
}
