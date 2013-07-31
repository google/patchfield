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

package com.noisepages.nettoyeur.patchbay.source;

import java.nio.ByteBuffer;

import android.app.Notification;

import com.noisepages.nettoyeur.patchbay.AudioModule;

public class PcmSource extends AudioModule {

  static {
    System.loadLibrary("pcmsource");
  }

  private long ptr = 0;
  private final int channels;
  private final ByteBuffer buffer;

  public PcmSource(int channels, ByteBuffer buffer, Notification notification) {
    super(notification);
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
  protected boolean configure(String name, int version, int token, int index, int sampleRate,
      int bufferSize) {
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
