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

import com.noisepages.nettoyeur.patchfield.IPatchfieldClient;

import android.app.Notification;

import java.util.List;

/**
 * Patchfield service interface, implemented by {@link Patchfield}.
 */
interface IPatchfieldService {
  
  /**
   * Registers a Patchfield client. Note that a client is not an audio module.
   * Rather, clients are Java objects that will be notified when the state
   * of the patchfield changes.
   */
  void registerClient(IPatchfieldClient client);
      
  /**
   * Unregisters a Patchfield client.
   */
  void unregisterClient(IPatchfieldClient client);
  
  /**
   * Starts audio rendering.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int start();
      
  /**
   * Stops audio rendering.
   */
  void stop();
      
  /**
   * Activates the given audio module.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int activateModule(String module);
      
  /**
   * Deactivates the given audio module.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int deactivateModule(String module);
      
  /**
   * Connects the given source port to the given sink port.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int connectPorts(String source, int sourcePort, String sink, int sinkPort);
      
  /**
   * Disconnects the given source port from the given sink port.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int disconnectPorts(String source, int sourcePort, String sink, int sinkPort);
      
  /**
   * @return True if the Patchfield is currently rendering audio.
   */
  boolean isRunning();
      
  /**
   * @return The list of currently registered audio modules. 
   */
  List<String> getModules();
      
  /**
   * @return The number of input channels of the given module, or a negative error
   * code if the module doesn't exist.
   */
  int getInputChannels(String module);
      
  /**
   * @return The number of output channels of the given module, or a negative error
   * code if the module doesn't exist.
   */
  int getOutputChannels(String module);

  /**
   * @return The notification associated with this module, or null if the module
   * doesn't exist. If module is null, it will return the master notification (see
   * setMasterNotification below).
   */
  Notification getNotification(String module);
  
  /**
   * @return True if the given module is currently active.
   */
  boolean isActive(String module);
      
  /**
   * @return True if the source port is directly connected to the sink port.
   */
  boolean isConnected(String source, int sourcePort, String sink, int sinkPort);
      
  /**
   * @return True if the input of of the given sink depends on the output of the given source,
   * directly or indirectly. In other words, this method returns true if the source must have
   * produced its output before the sink can run.
   */
  boolean isDependent(String sink, String source);
  
  /**
   * @return The sample rate in Hz at which the Patchfield operates. This value is determined
   * by the hardware and cannot be changed. Audio modules should be able to operate at 44.1kHz
   * as well as 48kHz.
   */
  int getSampleRate();
      
  /**
   * @return The buffer size in frames at which the Patchfield operates. This value is determined
   * by the hardware and cannot be changed. Audio modules should make no assumptions about the
   * buffer size. In particular, it will not be a power of two on many devices.
   */
  int getBufferSize();
  
  /**
   * Sets the master notification, which provides an intent for navigating from individual modules to an
   * optional control app. It also equips the service with foreground privileges, and the notification will
   * be used as the persistent notification that shows up in the notification bar.
   */
  void setMasterNotification(in Notification notification);
  
  /**
   * Creates a new audio module in the Patchfield service; for internal use mostly, to be called by
   * the configure method of {@link AudioModule}.
   *
   * @return The index of the new module on success, or a negative error code on failure.
   */
  int createModule(String module, int inputChannels, int outputChannels, in Notification notification);
      
  /**
   * Deletes an audio module from the Patchfield service; for internal use mostly, to be called by
   * the release method of {@link AudioModule}.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int deleteModule(String module);
      
  /**
   * @return The native protocol version; for internal use only.
   */
  int getProtocolVersion();
  
  /**
   * Passes the ashmem file descriptor through a Unix domain socket; for internal use only.
   *
   * @return 0 on success, or a negative error code on failure.
   */
  int sendSharedMemoryFileDescriptor();
}
