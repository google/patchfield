package com.noisepages.nettoyeur.patchbay.samples;

import com.noisepages.nettoyeur.patchbay.AudioModule;

/**
 * A sample audio module that implements a simple discretization of an RC lowpass
 * filter. The native components are in Patchbay/jni/samples/lowpass.c. They
 * illustrate a number of crucial points. In particular, they show how to use
 * the rendering context of the processing callback, and how to update
 * parameters in a lock-free yet thread-safe manner.
 */
public class LowpassModule extends AudioModule {

	static {
		System.loadLibrary("lowpass");
	}
	
	private long ptr = 0;
	private final int channels;
	
	public LowpassModule(int channels) {
		if (channels < 1 || channels > getMaxChannels()) {
			throw new IllegalArgumentException("Channel count out of range.");
		}
		this.channels = channels;
	}

	@Override
	protected boolean configure(String name, int token, int index) {
		if (ptr != 0) {
			throw new IllegalStateException("Module has already been configured.");
		}
		ptr = createModule(token, index, channels);
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
	protected int getInputChannels() {
		return channels;
	}

	@Override
	protected int getOutputChannels() {
		return channels;
	}
	
	/**
	 * Sets the cutoff frequency of the lowpass filter.
	 * 
	 * @param q cutoff frequency as a fraction of the sample rate.
	 */
	public void setCutoff(double q) {
		if (ptr == 0) {
			throw new IllegalStateException("Module is not configured.");
		}
		// Simple discretization of an RC lowpass filter; see, for example,
		// http://en.wikipedia.org/wiki/Low-pass_filter#Discrete-time_realization.
		double r = 2.0 * Math.PI * q;
		double alpha = r / (r + 1);
		setParameter(ptr, alpha);
	}

	@Override
	public boolean hasTimedOut() {
		if (ptr == 0) {
			throw new IllegalStateException("Module is not configured.");
		}
		return hasTimedOut(ptr);
	}
	
	public static native int getMaxChannels();
	
	private native void setParameter(long ptr, double alpha);
	private native long createModule(int token, int index, int channels);
	private native void release(long ptr);
	private native boolean hasTimedOut(long ptr);
}
