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

package com.noisepages.nettoyeur.patchfield.pd;

import org.puredata.core.PdBase;

import android.app.Notification;

import com.noisepages.nettoyeur.patchfield.AudioModule;

/**
 * An audio module implementation that uses Pure Data (via libpd) internally.
 * 
 * Since Pd is currently limited to one instance per process, PdModule is a singleton of sorts. When
 * it is first created, the channel counts are configurable; once it has been created, the
 * configuration is fixed for the lifetime of the process.
 * 
 * PdModule takes care of the initialization of libpd. In particular, make sure to create your
 * PdModule instance before calling any methods on PdBase. Do _not_ call PdBase.openAudio(...) or
 * PdBase.computeAudio(...). After creating your PdModule instance, you can use PdBase as usual.
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

  /**
   * Static factory method for creating PdModule instances. Note that the sample rate must be the
   * sample rate reported by the Patchfield service.
   */
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
  public int getInputChannels() {
    return inputChannels;
  }

  @Override
  public int getOutputChannels() {
    return outputChannels;
  }

  @Override
  protected boolean configure(String name, long handle, int sampleRate, int bufferSize) {
    if (ptr != 0) {
      throw new IllegalStateException("Module has already been configured.");
    }
    ptr = configureModule(handle, bufferSize, PdBase.blockSize(), inputChannels, outputChannels);
    return ptr != 0;
  }

  @Override
  protected void release() {
    if (ptr != 0) {
      release(ptr);
    }
  }

  private native void pdInitAudio(int inputChannels, int outputChannels, int sampleRate);

  private native long configureModule(long handle, int bufferSize, int blockSize,
      int inputChannels, int outputChannels);

  private native void release(long ptr);
}
