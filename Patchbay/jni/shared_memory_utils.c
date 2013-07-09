#include "shared_memory_utils.h"

#include "shared_memory_internal.h"

#include <unistd.h>

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchbay_SharedMemoryUtils_receiveSharedMemoryFileDescriptor
(JNIEnv *env, jclass cls) {
  return smi_receive();
}

JNIEXPORT jint JNICALL
Java_com_noisepages_nettoyeur_patchbay_SharedMemoryUtils_closeSharedMemoryFileDescriptor
(JNIEnv *env, jclass cls, jint fd) {
  return close(fd);
}
