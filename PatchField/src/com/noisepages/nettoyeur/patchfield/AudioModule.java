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
 * Abstract base class for Patchfield audio modules. Subclasses must implement
 * methods for creating and releasing audio modules; these implementations will
 * involve native code using the native audio_module library in Patchfield/jni.
 * See the LowpassSample project for a representative audio module
 * implementation.
 * 
 * The Patchfield service operates at the native sample rate and buffer size of
 * the device. This means that audio modules must operate at the native sample
 * rate and buffer size as well. Native sample rates of 44100Hz and 48000Hz are
 * common, and so audio modules must support both. Moreover, audio modules must
 * be prepared to work with arbitrary buffer sizes. In particular, they cannot
 * assume that the buffer size is a power of two. Multiples of three, such as
 * 144, 192, and 384, have been seen in the wild.
 * 
 * If an app is unable to run at the native buffer size, the buffer size adapter
 * utility in Patchfield/jni/utils/buffer_size_adapter.{h,c} can be used. For an
 * example of the buffer size adapter in action, see the PatchfieldPd library
 * project.
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
   * Constructor.
   * 
   * @param notification
   *            Notification to be passed to the Patchfield service, so that the service
   *            can associate an audio module with an app. May be null.
   */
  protected AudioModule(Notification notification) {
    this.notification = notification;
  }

  /**
   * This method takes care of the elaborate choreography that it takes to set
   * up an audio module and to connect it to its representation in the
   * Patchfield service.
   * 
   * Specifically, it sets up the shared memory between the local module and
   * the Patchfield service, creates a new module in the service, and connects
   * it to the local module.
   * 
   * A module can only be configured once. If it times out, it cannot be
   * reinstated and should be released.
   * 
   * @param patchfield
   *            Stub for communicating with the Patchfield service.
   * @param name
   *            Name of the new audio module in Patchfield.
   * @return 0 on success, a negative error on failure; use
   *         {@link PatchfieldException} to interpret the return value.
   * @throws RemoteException
   */
  public int configure(IPatchfieldService patchfield, String name)
      throws RemoteException {
    int version = patchfield.getProtocolVersion();
    if (version != getProtocolVersion()) {
      return PatchfieldException.PROTOCOL_VERSION_MISMATCH;
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
    int index = patchfield.createModule(name, getInputChannels(),
        getOutputChannels(), notification);
    if (index < 0) {
      SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
      return index;
    }
    handle = createRunner(version, token, index);
    if (handle == 0) {
      patchfield.deleteModule(name);
      SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
      return PatchfieldException.FAILURE;
    }
    if (!configure(name, handle, patchfield.getSampleRate(),
        patchfield.getBufferSize())) {
      release(handle);
      patchfield.deleteModule(name);
      SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
      return PatchfieldException.FAILURE;
    }
    this.name = name;
    return PatchfieldException.SUCCESS;
  }

  /**
   * Releases all resources associated with this module and deletes its
   * representation in the Patchfield service.
   * 
   * @param patchfield
   *            Stub for communicating with the Patchfield service.
   * @throws RemoteException
   */
  public void release(IPatchfieldService patchfield) throws RemoteException {
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

  /**
   * @return The notification associated with this module.
   */
  protected Notification getNotification() {
    return notification;
  }

  /**
   * @return True if the module has timed out. A module that has timed out
   *         cannot be reinstated and must be released.
   */
  public final boolean hasTimedOut() {
    return handle != 0 && hasTimedOut(handle);
  }

  /**
   * @return The number of input channels of this module.
   */
  public abstract int getInputChannels();

  /**
   * @return The number of output channels of this module.
   */
  public abstract int getOutputChannels();

  /**
   * This method is called by the public configure method. It is responsible
   * to setting up the native components of an audio module implementation,
   * such as the audio processing function and any data structures that make
   * up the processing context.
   * 
   * @param name
   * @param handle
   *            Opaque handle to the internal data structure representing an
   *            audio module.
   * @param sampleRate
   * @param bufferSize
   * @return True on success
   */
  protected abstract boolean configure(String name, long handle,
      int sampleRate, int bufferSize);

  /**
   * Releases any resources held by the audio module, such as memory allocated
   * for the processing context.
   */
  protected abstract void release();

  /**
   * @return The Patchfield protocol version; mostly for internal use.
   */
  public static native int getProtocolVersion();

  private native long createRunner(int version, int token, int index);

  private native void release(long handle);

  private native boolean hasTimedOut(long handle);
}
