package com.noisepages.nettoyeur.patchbay.pd;

import org.puredata.core.PdBase;

import android.app.PendingIntent;

import com.noisepages.nettoyeur.patchbay.AudioModule;

public class PdModule extends AudioModule {

	static {
		PdBase.blockSize();  // Make sure to load the class PdBase first.
		System.loadLibrary("pdmodule");
	}

	private long ptr = 0;

	private final int inputChannels;
	private final int outputChannels;
	
	PdModule(int inputChannels, int outputChannels, PendingIntent intent) {
		super(intent);
		this.inputChannels = inputChannels;
		this.outputChannels = outputChannels;
	}

	@Override
	public boolean hasTimedOut() {
		if (ptr == 0) {
			throw new IllegalStateException("Module is not configured.");
		}
		return false;
	}

	@Override
	public int getProtocolVersion() {
		if (ptr == 0) {
			throw new IllegalStateException("Module is not configured.");
		}
		return 0;
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
		return false;
	}

	@Override
	protected void release() {
	}
}