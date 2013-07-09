package com.noisepages.nettoyeur.patchbay.samples;

import com.noisepages.nettoyeur.patchbay.AudioModule;

/**
 * A simple example that shows how to set up a Patchbay audio module with the
 * least amount of code possible. The native components are in
 * Patchbay/jni/samples/identity.c. For a more realistic example, see
 * {@link LowpassModule}.
 */
public class IdentityModule extends AudioModule {

	static {
		System.loadLibrary("identity");
	}
	
	private long ptr = 0;
	
	@Override
	protected int getInputChannels() {
		return 1;
	}

	@Override
	protected int getOutputChannels() {
		return 1;
	}

	@Override
	protected boolean configure(String name, int token, int index) {
		if (ptr != 0) {
			throw new IllegalStateException("Module has already been configured.");
		}
		ptr = createModule(token, index);
		return ptr != 0;
	}

	@Override
	protected void release() {
		if (ptr != 0) {
			release(ptr);
			ptr = 0;
		}
	}

	@Override
	public boolean hasTimedOut() {
		if (ptr == 0) {
			throw new IllegalStateException("Module is not configured.");
		}
		return hasTimedOut(ptr);
	}

	private native long createModule(int token, int index);
	private native void release(long ptr);
	private native boolean hasTimedOut(long ptr);
}
