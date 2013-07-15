package com.noisepages.nettoyeur.patchbay.samples;

import android.app.PendingIntent;

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
	
	public IdentityModule(PendingIntent intent) {
		super(intent);
	}

	@Override
	public int getInputChannels() {
		return 1;
	}

	@Override
	public int getOutputChannels() {
		return 1;
	}

	@Override
	protected boolean configure(String name, int version, int token, int index, int sampleRate, int bufferSize) {
		if (ptr != 0) {
			throw new IllegalStateException("Module has already been configured.");
		}
		ptr = createModule(version, token, index);
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

	@Override
	public native int getProtocolVersion();

	private native long createModule(int version, int token, int index);
	private native void release(long ptr);
	private native boolean hasTimedOut(long ptr);
}
