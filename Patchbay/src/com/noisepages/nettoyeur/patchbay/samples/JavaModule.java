package com.noisepages.nettoyeur.patchbay.samples;

import android.app.PendingIntent;

import com.noisepages.nettoyeur.patchbay.AudioModule;

public abstract class JavaModule extends AudioModule {

  static {
    System.loadLibrary("javamodule");
  }

  private final int inputChannels;
  private final int outputChannels;

  private long ptr = 0;

  private float[] inputBuffer = null;
  private float[] outputBuffer = null;
  private int sampleRate;
  private int bufferSize;
  
  private Thread renderThread = null;

  private final Runnable processor = new Runnable() {
    @Override
    public void run() {
      while (!hasTimedOut(ptr)) {
        fillInputBuffer(ptr, inputBuffer);
        if (Thread.interrupted()) {
          break;
        }
        process(sampleRate, bufferSize, inputChannels, inputBuffer, outputChannels, outputBuffer);
        sendOutputBuffer(ptr, outputBuffer);
      }
    }
  };

  public JavaModule(int inputChannels, int outputChannels, PendingIntent intent) {
    super(intent);
    this.inputChannels = inputChannels;
    this.outputChannels = outputChannels;
  }

  protected abstract void process(int sampleRate, int bufferSize,
                                  int inputChannels, float[] inputBuffer,
                                  int outputChannels, float[] outputBuffer);

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
    inputBuffer = new float[bufferSize * inputChannels];
    outputBuffer = new float[bufferSize * outputChannels];
    this.sampleRate = sampleRate;
    this.bufferSize = bufferSize;
    ptr = configure(version, token, index);
    if (ptr != 0) {
      renderThread = new Thread(processor);
      renderThread.start();
      return true;
    } else {
      return false;
    }
  }

  @Override
  protected void release() {
    if (renderThread != null) {
      signalThread(ptr);
      renderThread.interrupt();
      try {
        renderThread.join();
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    }
    if (ptr != 0) {
      release(ptr);
      ptr = 0;
    }
  }

  @Override
  public native int getProtocolVersion();

  private native boolean hasTimedOut(long ptr);

  private native long configure(int version, int token, int index);

  private native void release(long ptr);

  private native void fillInputBuffer(long ptr, float[] inputBuffer);

  private native void sendOutputBuffer(long ptr, float[] outputBuffer);

  private native void signalThread(long ptr);
}
