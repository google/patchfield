package com.noisepages.nettoyeur.patchbay.source;

import java.nio.ByteBuffer;

import com.noisepages.nettoyeur.patchbay.AudioModule;

public class PcmSource extends AudioModule {

	static {
		System.loadLibrary("pcmsource");
	}
	
	private long ptr = 0;
	private final int channels;
	private final ByteBuffer buffer;
	
	public PcmSource(int channels, ByteBuffer buffer) {
		this.channels = channels;
		this.buffer = buffer;
	}
	
	@Override
	public int getInputChannels() {
		return 0;
	}

	@Override
	public int getOutputChannels() {
		return channels;
	}

	@Override
	protected boolean configure(String name, int version, int token, int index) {
		ptr = createSource(version, token, index, buffer);
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
	
	private native long createSource(int version, int token, int index, ByteBuffer buffer);
	private native boolean hasTimedOut(long ptr);
	private native void release(long ptr);
}
