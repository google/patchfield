/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.noisepages.nettoyeur.patchfield.modules;

import android.app.Notification;

import com.noisepages.nettoyeur.patchfield.AudioModule;

/**
 * An audio module subclass whose audio processing callback is to be implemented
 * in Java (as opposed to native code).
 * 
 * THIS IS NOT THE RECOMMENDED WAY TO USE PATCHFIELD. The Java processing callback
 * cannot be invoked on a real-time thread and Java code is prone to garbage
 * collection breaks. This means that instances of this class run a higher risk
 * of missing their deadlines and causing dropouts than audio modules that do
 * their processing natively. Use this class for quick-and-dirty prototypes,
 * but not for any serious applications that require glitch-free performance.
 */
public abstract class JavaModule extends AudioModule {

  static {
    System.loadLibrary("javamodule");
  }

  private final int inputChannels;
  private final int outputChannels;
  private final int bufferSize;
  private float[] inputBuffer = null;
  private float[] outputBuffer = null;

  private long ptr = 0;
  private int sampleRate;
  private Thread renderThread = null;

  private final Runnable processor = new Runnable() {
    @Override
    public void run() {
      while (!hasTimedOut()) {
        fillInputBuffer(ptr, inputBuffer);
        if (Thread.interrupted()) {
          break;
        }
        process(sampleRate, bufferSize, inputChannels, inputBuffer, outputChannels, outputBuffer);
        sendOutputBuffer(ptr, outputBuffer);
      }
    }
  };

  /**
   * Constructor. For best performance, make the buffer size equal to Patchfield.getBufferSize(). When
   * this is not an option, choose a smallish buffer size if possible (64 is a good value). Large
   * buffers will not improve stability. In fact, large buffers may increase the risk of dropouts
   * because the patchfield runs a fixed buffer size internally; mismatched buffer sizes place an
   * uneven load on the internal processing callback.
   */
  public JavaModule(int bufferSize, int inputChannels, int outputChannels, Notification notification) {
    super(notification);
    this.bufferSize = bufferSize;
    this.inputChannels = inputChannels;
    this.outputChannels = outputChannels;
    inputBuffer = new float[bufferSize * inputChannels];
    outputBuffer = new float[bufferSize * outputChannels];
  }

  /**
   * Audio processing callback. The size of each buffer is the buffer size (in frames) times the
   * number of channels. Buffers are non-interleaved.
   */
  protected abstract void process(int sampleRate, int bufferSize, int inputChannels,
      float[] inputBuffer, int outputChannels, float[] outputBuffer);

  @Override
  public int getInputChannels() {
    return inputChannels;
  }

  @Override
  public int getOutputChannels() {
    return outputChannels;
  }

  @Override
  protected boolean configure(String name, long handle, int sampleRate, int hostBufferSize) {
    this.sampleRate = sampleRate;
    ptr = configure(handle, hostBufferSize, bufferSize, inputChannels, outputChannels);
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
      renderThread.interrupt();
      signalThread(ptr);
      try {
        renderThread.join();
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
      renderThread = null;
    }
    if (ptr != 0) {
      release(ptr);
      ptr = 0;
    }
  }

  private native long configure(long handle, int hostBufferSize, int bufferSize, int inputChannels,
      int outputChannels);

  private native void release(long ptr);

  private native void fillInputBuffer(long ptr, float[] inputBuffer);

  private native void sendOutputBuffer(long ptr, float[] outputBuffer);

  private native void signalThread(long ptr);
}
