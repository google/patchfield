package com.noisepages.nettoyeur.patchbay;

/**
 * Utilities for receiving file descriptors from the Patchbay service; package
 * private because client code will not need to call it directly.
 */
class SharedMemoryUtils {

	static {
		System.loadLibrary("shared_memory_utils");
	}
	
	static native int receiveSharedMemoryFileDescriptor();
	static native int closeSharedMemoryFileDescriptor(int fd);
}
