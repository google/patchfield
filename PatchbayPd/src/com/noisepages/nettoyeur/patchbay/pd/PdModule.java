package com.noisepages.nettoyeur.patchbay.pd;

import org.puredata.core.PdBase;

import android.app.Notification;

import com.noisepages.nettoyeur.patchbay.AudioModule;

/**
 * An audio module implementation that uses Pure Data (via libpd) internally.
 * 
 * Since Pd is currently limited to one instance per process, PdModule is a singleton of sorts. When
 * it is first created, the sample rate and channel counts are configurable; once it has been
 * created, the configuration is fixed for the lifetime of the process.
 * 
 * PdModule takes care of the initialization of libpd. In particular, make sure to create your
 * PdModule instance before calling any methods on PdBase. Do _not_ call PdBase.openAudio(...) of
 * PdBase.computeAudio(...). After the creation of your PdModule instance, you can use PdBase as
 * usual.
 */
public class PdModule extends AudioModule {

  static {
    PdBase.blockSize(); // Make sure to load PdBase first.
    System.loadLibrary("pdmodule");
  }

  private static PdModule instance = null;

  private long ptr = 0;

  private final int sampleRate;
  private final int inputChannels;
  private final int outputChannels;

  private PdModule(int sampleRate, int inputChannels, int outputChannels, Notification notification) {
    super(notification);
    this.sampleRate = sampleRate;
    this.inputChannels = inputChannels;
    this.outputChannels = outputChannels;
    pdInitAudio(inputChannels, outputChannels, sampleRate);
    PdBase.computeAudio(true);
  }

  public static PdModule getInstance(int sampleRate, int inputChannels, int outputChannels,
      Notification notification) {
    if (instance == null) {
      return new PdModule(sampleRate, inputChannels, outputChannels, notification);
    } else if (instance.getInputChannels() >= inputChannels
        && instance.getOutputChannels() >= outputChannels
        && (notification == null || notification.equals(instance.getNotification()))) {
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
  public int getInputChannels() {
    return inputChannels;
  }

  @Override
  public int getOutputChannels() {
    return outputChannels;
  }

  @Override
  protected boolean configure(String name, int version, int token, int index, int sampleRate,
      int bufferSize) {
    if (ptr != 0) {
      throw new IllegalStateException("Module has already been configured.");
    }
    ptr =
        configureModule(version, token, index, bufferSize, PdBase.blockSize(), inputChannels,
            outputChannels);
    return ptr != 0;
  }

  @Override
  protected void release() {
    if (ptr != 0) {
      release(ptr);
    }
  }

  @Override
  public native int getProtocolVersion();

  private native void pdInitAudio(int inputChannels, int outputChannels, int sampleRate);

  private native boolean hasTimedOut(long ptr);

  private native long configureModule(int version, int token, int index, int bufferSize,
      int blockSize, int inputChannels, int outputChannels);

  private native void release(long ptr);
}
