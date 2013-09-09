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

package com.noisepages.nettoyeur.patchfield.source;

import java.nio.ByteBuffer;

import android.app.Notification;

import com.noisepages.nettoyeur.patchfield.AudioModule;

/**
 * Audio module for playing sound files in PCM format.
 */
public class PcmSource extends AudioModule {

  static {
    System.loadLibrary("pcmsource");
  }

  private long ptr = 0;
  private final int channels;
  private final ByteBuffer buffer;

  /**
   * Creates a new PCM source.
   * 
   * @param channels Number of output channels.
   * @param buffer Direct byte buffer holding PCM data as interleaved 32-bit floats.
   * @param notification
   */
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
  protected boolean configure(String name, long handle, int sampleRate, int bufferSize) {
    ptr = createSource(handle, buffer);
    return ptr != 0;
  }

  @Override
  protected void release() {
    if (ptr != 0) {
      release(ptr);
      ptr = 0;
    }
  }

  private native long createSource(long handle, ByteBuffer buffer);

  private native void release(long ptr);
}
