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

package com.noisepages.nettoyeur.patchfield.service;

import java.io.IOException;
import java.util.List;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;

import com.noisepages.nettoyeur.patchfield.IPatchfieldClient;
import com.noisepages.nettoyeur.patchfield.IPatchfieldService;
import com.noisepages.nettoyeur.patchfield.Patchfield;

/**
 * Boilerplate for turning {@link Patchfield} into an Android service.
 */
public class PatchfieldService extends Service {

  private Patchfield patchfield = null;

  private final IPatchfieldService.Stub binder = new IPatchfieldService.Stub() {

    @Override
    public int sendSharedMemoryFileDescriptor() {
      return patchfield.sendSharedMemoryFileDescriptor();
    }

    @Override
    public void unregisterClient(IPatchfieldClient client) throws RemoteException {
      patchfield.unregisterClient(client);
    }

    @Override
    public void registerClient(IPatchfieldClient client) throws RemoteException {
      patchfield.registerClient(client);
    }

    @Override
    public int getSampleRate() {
      return patchfield.getSampleRate();
    }

    @Override
    public int getBufferSize() {
      return patchfield.getBufferSize();
    }

    @Override
    public int deleteModule(String module) throws RemoteException {
      return patchfield.deleteModule(module);
    }

    @Override
    public int createModule(String module, int inputChannels, int outputChannels,
        Notification notification) throws RemoteException {
      return patchfield.createModule(module, inputChannels, outputChannels, notification);
    }

    @Override
    public int connectPorts(String source, int sourcePort, String sink, int sinkPort)
        throws RemoteException {
      return patchfield.connectPorts(source, sourcePort, sink, sinkPort);
    }

    @Override
    public int disconnectPorts(String source, int sourcePort, String sink, int sinkPort)
        throws RemoteException {
      return patchfield.disconnectPorts(source, sourcePort, sink, sinkPort);
    }

    @Override
    public List<String> getModules() throws RemoteException {
      return patchfield.getModules();
    }

    @Override
    public int getInputChannels(String module) throws RemoteException {
      return patchfield.getInputChannels(module);
    }

    @Override
    public int getOutputChannels(String module) throws RemoteException {
      return patchfield.getOutputChannels(module);
    }

    @Override
    public Notification getNotification(String module) throws RemoteException {
      return patchfield.getNotification(module);
    }

    @Override
    public boolean isConnected(String source, int sourcePort, String sink, int sinkPort)
        throws RemoteException {
      return patchfield.isConnected(source, sourcePort, sink, sinkPort);
    }

    @Override
    public boolean isDependent(String source, String sink) throws RemoteException {
      return patchfield.isDependent(source, sink);
    }

    @Override
    public int start() {
      return patchfield.start();
    }

    @Override
    public void stop() {
      patchfield.stop();
    }

    @Override
    public boolean isRunning() {
      return patchfield.isRunning();
    }

    @Override
    public int activateModule(String module) throws RemoteException {
      return patchfield.activateModule(module);
    }

    @Override
    public int deactivateModule(String module) throws RemoteException {
      return patchfield.deactivateModule(module);
    }

    @Override
    public boolean isActive(String module) throws RemoteException {
      return patchfield.isActive(module);
    }

    @Override
    public int getProtocolVersion() throws RemoteException {
      return patchfield.getProtocolVersion();
    }

    @Override
    public void setMasterNotification(Notification notification) throws RemoteException {
      patchfield.setMasterNotification(notification);
      PatchfieldService.this.startForeground(Integer.MAX_VALUE, notification);
    }
  };

  @Override
  public void onCreate() {
    super.onCreate();
    try {
      patchfield = new Patchfield(this, 2, 2);
    } catch (IOException e) {
      patchfield = null;
    }
  };

  @Override
  public IBinder onBind(Intent intent) {
    return (patchfield != null) ? binder : null;
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    release();
  }

  private void release() {
    if (patchfield != null) {
      patchfield.release();
      patchfield = null;
    }
  }
}
