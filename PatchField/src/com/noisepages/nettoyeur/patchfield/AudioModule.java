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

package com.noisepages.nettoyeur.patchfield;

import android.app.Notification;
import android.os.RemoteException;
import android.util.Log;

import com.noisepages.nettoyeur.patchfield.internal.SharedMemoryUtils;

/**
 * Abstract base class for PatchField audio modules. Subclasses must implement methods for creating
 * and releasing audio modules; these implementations will involve native code using the native
 * audio_module library in PatchField/jni.
 * 
 * The PatchField service operates at the native sample rate and buffer size of the device. This means
 * that audio modules must operate at the native sample rate and buffer size as well. Native sample
 * rates of 44100Hz and 48000Hz are common, and so audio modules must support both. Moreover, audio
 * modules must be prepared to work with arbitrary buffer sizes. In particular, they cannot assume
 * that the buffer size is a power of two. Multiples of three, such as 144, 192, and 384, have been
 * seen in the wild.
 * 
 * If an app is unable to run at the native buffer size, the buffer size adapter utility in
 * PatchField/jni/utils/buffer_size_adapter.{h,c} can be used. For an example of the buffer size
 * adapter in action, see the PatchFieldPd project.
 */
public abstract class AudioModule {

  static {
    System.loadLibrary("audiomodulejava");
  }

  private static final String TAG = "AudioModule";

  private String name = null;
  private int token = -1;
  private long handle = 0;

  private final Notification notification;

  private class FdReceiverThread extends Thread {
    @Override
    public void run() {
      token = SharedMemoryUtils.receiveSharedMemoryFileDescriptor();
      Log.i(TAG, "fd: " + token);
    }
  }

  /**
   * @param notification to be passed to the PatchField service, so that the service can associate an
   *        audio module with an app. May be null.
   */
  protected AudioModule(Notification notification) {
    this.notification = notification;
  }

  /**
   * This method takes care of the elaborate choreography that it takes to set up an audio module
   * and to connect it to its representation in the PatchField service.
   * 
   * Specifically, it sets up the shared memory between the local module and the PatchField service,
   * creates a new module in the service, and connects it to the local module.
   * 
   * A module can only be configured once. If it times out, it cannot be reinstated and should be
   * released.
   * 
   * @param patchfield stub for communicating with the PatchField service
   * @param name of the new audio module in PatchField
   * @return 0 on success, a negative error on failure; use {@link PatchFieldException} to interpret
   *         the return value.
   * @throws RemoteException
   */
  public int configure(IPatchFieldService patchfield, String name) throws RemoteException {
    int version = patchfield.getProtocolVersion();
    if (version != getProtocolVersion()) {
      return PatchFieldException.PROTOCOL_VERSION_MISMATCH;
    }
    if (this.handle != 0) {
      throw new IllegalStateException("Module is already configured.");
    }
    FdReceiverThread t = new FdReceiverThread();
    t.start();
    while (patchfield.sendSharedMemoryFileDescriptor() != 0 && t.isAlive()) {
      try {
        Thread.sleep(10); // Wait for receiver thread to spin up.
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    }
    try {
      t.join();
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
    if (token < 0) {
      return token;
    }
    int index = patchfield.createModule(name, getInputChannels(), getOutputChannels(), notification);
    if (index < 0) {
      SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
      return index;
    }
    handle = createRunner(version, token, index);
    if (handle == 0) {
      patchfield.deleteModule(name);
      SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
      return PatchFieldException.FAILURE;
    }
    if (!configure(name, handle, patchfield.getSampleRate(), patchfield.getBufferSize())) {
      release(handle);
      patchfield.deleteModule(name);
      SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
      return PatchFieldException.FAILURE;
    }
    this.name = name;
    return PatchFieldException.SUCCESS;
  }

  /**
   * Releases all resources associated with this module and deletes its representation in the
   * PatchField service.
   * 
   * @param patchfield stub for communicating with the PatchField service
   * @throws RemoteException
   */
  public void release(IPatchFieldService patchfield) throws RemoteException {
    if (handle != 0) {
      patchfield.deleteModule(name);
      release(handle);
      release();
      SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
      name = null;
      handle = 0;
      token = -1;
    } else {
      Log.w(TAG, "Not configured; nothing to release.");
    }
  }

  /**
   * @return The name of the module if configured, null if it isn't.
   */
  public String getName() {
    return name;
  }

  protected Notification getNotification() {
    return notification;
  }

  public final boolean hasTimedOut() {
    return handle != 0 && hasTimedOut(handle);
  }

  public abstract int getInputChannels();

  public abstract int getOutputChannels();

  protected abstract boolean configure(String name, long handle, int sampleRate, int bufferSize);

  protected abstract void release();

  public static native int getProtocolVersion();

  private native long createRunner(int version, int token, int index);

  private native void release(long handle);

  private native boolean hasTimedOut(long handle);
}
