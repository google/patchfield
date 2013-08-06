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
  protected boolean configure(String name, long handle, int sampleRate, int bufferSize) {
    return configure(handle);
  }

  @Override
  protected void release() {
    // Nothing to do.
  }

  private native boolean configure(long handle);
}
