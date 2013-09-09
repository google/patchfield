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

package com.noisepages.nettoyeur.patchfield.lowpass;

import android.app.Notification;

import com.noisepages.nettoyeur.patchfield.AudioModule;

/**
 * A sample audio module that implements a simple discretization of an RC lowpass filter. The native
 * components are in Patchfield/jni/samples/lowpass.c. They illustrate a number of crucial points.
 * In particular, they show how to use the rendering context of the processing callback, and how to
 * update parameters in a lock-free yet thread-safe manner.
 */
public class LowpassModule extends AudioModule {

  static {
    System.loadLibrary("lowpass");
  }

  private long ptr = 0;
  private final int channels;

  public LowpassModule(int channels, Notification notification) {
    super(notification);
    if (channels < 1) {
      throw new IllegalArgumentException("Channel count must be at least one.");
    }
    this.channels = channels;
  }

  @Override
  protected boolean configure(String name, long handle, int sampleRate, int bufferSize) {
    if (ptr != 0) {
      throw new IllegalStateException("Module has already been configured.");
    }
    ptr = configureNativeComponents(handle, channels);
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
  public int getInputChannels() {
    return channels;
  }

  @Override
  public int getOutputChannels() {
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

  private native long configureNativeComponents(long handle, int channels);

  private native void release(long ptr);

  private native void setParameter(long ptr, double alpha);
}
