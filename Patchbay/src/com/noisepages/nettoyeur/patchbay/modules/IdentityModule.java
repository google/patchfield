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

package com.noisepages.nettoyeur.patchbay.modules;

import android.app.Notification;

import com.noisepages.nettoyeur.patchbay.AudioModule;

/**
 * A simple example that shows how to set up a Patchbay audio module with the least amount of code
 * possible. The native components are in Patchbay/jni/samples/identity.c. For a more realistic
 * example, see {@link LowpassModule}.
 */
public class IdentityModule extends AudioModule {

  static {
    System.loadLibrary("identity");
  }

  private long ptr = 0;

  public IdentityModule(Notification notification) {
    super(notification);
  }

  @Override
  public int getInputChannels() {
    return 1;
  }

  @Override
  public int getOutputChannels() {
    return 1;
  }

  @Override
  protected boolean configure(String name, int version, int token, int index, int sampleRate,
      int bufferSize) {
    if (ptr != 0) {
      throw new IllegalStateException("Module has already been configured.");
    }
    ptr = createModule(version, token, index);
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

  private native long createModule(int version, int token, int index);

  private native void release(long ptr);

  private native boolean hasTimedOut(long ptr);
}
