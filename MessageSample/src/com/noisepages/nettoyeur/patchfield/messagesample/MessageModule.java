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

package com.noisepages.nettoyeur.patchfield.messagesample;

import android.app.Notification;

import com.noisepages.nettoyeur.patchfield.AudioModule;

public class MessageModule extends AudioModule {

  static {
    System.loadLibrary("message");
  }

  private long ptr = 0;

  public MessageModule(Notification notification) {
    super(notification);
  }

  @Override
  protected boolean configure(String name, long handle, int sampleRate, int bufferSize) {
    if (ptr != 0) {
      throw new IllegalStateException("Module has already been configured.");
    }
    ptr = configureNativeComponents(handle);
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
    return 0;
  }

  @Override
  public int getOutputChannels() {
    return 1;
  }

  private native long configureNativeComponents(long handle);

  private native void release(long ptr);
}
