package com.noisepages.nettoyeur.patchbay.pd;

import org.puredata.core.PdBase;

import android.app.PendingIntent;

import com.noisepages.nettoyeur.patchbay.AudioModule;

public class PdModule extends AudioModule {

	static {
		PdBase.blockSize();  // Make sure to load PdBase first.
		System.loadLibrary("pdmodule");
	}

	private static PdModule instance = null;
	
	private long ptr = 0;

	private final int inputChannels;
	private final int outputChannels;
	
	private PdModule(int inputChannels, int outputChannels, PendingIntent intent) {
		super(intent);
		this.inputChannels = inputChannels;
		this.outputChannels = outputChannels;
	}

	public static PdModule getInstance(int inputChannels, int outputChannels, PendingIntent intent) {
		if (instance == null) {
			return new PdModule(inputChannels, outputChannels, intent);
		} else if (instance.getInputChannels() >= inputChannels && instance.getOutputChannels() >= outputChannels
				&& (intent == null || intent.equals(instance.getIntent()))) {
			return instance;
		} else {
			throw new IllegalStateException("PdModule instance can't be reconfigured once instantiated.");
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
	public int getProtocolVersion() {
		return getProtocolVersion(ptr);
	}

	@Override
	public int getInputChannels() {
		return inputChannels;
	}

	@Override
	public int getOutputChannels() {
		return outputChannels;
	}

	@Override
	protected boolean configure(String name, int version, int token, int index, int sampleRate, int bufferSize) {
		if (ptr != 0) {
			throw new IllegalStateException("Module has already been configured.");
		}
		ptr = configureModule(version, token, index, bufferSize, PdBase.blockSize(), inputChannels, outputChannels);
		return ptr != 0;
	}

	@Override
	protected void release() {
		if (ptr != 0) {
			release(ptr);
		}
	}

	private native boolean hasTimedOut(long ptr);
	private native int getProtocolVersion(long ptr);
	private native long configureModule(int version, int token, int index, int bufferSize, int blockSize,
			int inputChannels, int outputChannels);
	private native void release(long ptr);
}