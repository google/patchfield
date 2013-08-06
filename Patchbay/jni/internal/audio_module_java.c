#include "audio_module_java.h"

#include "audio_module_internal.h"

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchbay_AudioModule_getProtocolVersion
(JNIEnv *env, jclass cls) {
  return PATCHBAY_PROTOCOL_VERSION;
}

JNIEXPORT jlong JNICALL
Java_com_noisepages_nettoyeur_patchbay_AudioModule_configure
(JNIEnv *env, jobject obj, jint version, jint token, jint index) {
  return (jlong) ami_create(version, token, index);
}

JNIEXPORT void JNICALL
Java_com_noisepages_nettoyeur_patchbay_AudioModule_release
(JNIEnv *env, jobject obj, jlong p) {
  audio_module_runner *amr = (audio_module_runner *) p;
  ami_release(amr);
}

JNIEXPORT jboolean JNICALL
Java_com_noisepages_nettoyeur_patchbay_AudioModule_hasTimedOut
(JNIEnv *env, jobject obj, jlong p) {
  audio_module_runner *amr = (audio_module_runner *) p;
  return ami_has_timed_out(amr);
}
