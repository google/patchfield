package com.noisepages.nettoyeur.patchbay.internal;

/**
 * Utilities for receiving file descriptors from the Patchbay service.
 */
public class SharedMemoryUtils {

  static {
    System.loadLibrary("shared_memory_utils");
  }

  public static native int receiveSharedMemoryFileDescriptor();

  public static native int closeSharedMemoryFileDescriptor(int fd);
}
