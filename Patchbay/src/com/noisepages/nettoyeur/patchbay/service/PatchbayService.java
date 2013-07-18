package com.noisepages.nettoyeur.patchbay.service;

import java.io.IOException;
import java.util.List;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;

import com.noisepages.nettoyeur.patchbay.IPatchbayClient;
import com.noisepages.nettoyeur.patchbay.IPatchbayService;
import com.noisepages.nettoyeur.patchbay.Patchbay;

/**
 * Boilerplate for turning {@link Patchbay} into an Android service.
 */
public class PatchbayService extends Service {

  private Patchbay patchbay = null;

  private final IPatchbayService.Stub binder = new IPatchbayService.Stub() {

    @Override
    public int sendSharedMemoryFileDescriptor() {
      return patchbay.sendSharedMemoryFileDescriptor();
    }

    @Override
    public void unregisterClient(IPatchbayClient client) throws RemoteException {
      patchbay.unregisterClient(client);
    }

    @Override
    public void registerClient(IPatchbayClient client) throws RemoteException {
      patchbay.registerClient(client);
    }

    @Override
    public int getSampleRate() {
      return patchbay.getSampleRate();
    }

    @Override
    public int getBufferSize() {
      return patchbay.getBufferSize();
    }

    @Override
    public int deleteModule(String module) throws RemoteException {
      return patchbay.deleteModule(module);
    }

    @Override
    public int createModule(String module, int inputChannels, int outputChannels,
        Notification notification) throws RemoteException {
      return patchbay.createModule(module, inputChannels, outputChannels, notification);
    }

    @Override
    public int connectModules(String source, int sourcePort, String sink, int sinkPort)
        throws RemoteException {
      return patchbay.connectModules(source, sourcePort, sink, sinkPort);
    }

    @Override
    public int disconnectModules(String source, int sourcePort, String sink, int sinkPort)
        throws RemoteException {
      return patchbay.disconnectModules(source, sourcePort, sink, sinkPort);
    }

    @Override
    public List<String> getModules() throws RemoteException {
      return patchbay.getModules();
    }

    @Override
    public int getInputChannels(String module) throws RemoteException {
      return patchbay.getInputChannels(module);
    }

    @Override
    public int getOutputChannels(String module) throws RemoteException {
      return patchbay.getOutputChannels(module);
    }

    @Override
    public Notification getNotification(String module) throws RemoteException {
      return patchbay.getNotification(module);
    }

    @Override
    public boolean isConnected(String source, int sourcePort, String sink, int sinkPort)
        throws RemoteException {
      return patchbay.isConnected(source, sourcePort, sink, sinkPort);
    }

    @Override
    public boolean isDependent(String source, String sink) throws RemoteException {
      return patchbay.isDependent(source, sink);
    }

    @Override
    public int start() {
      return patchbay.start();
    }

    @Override
    public void stop() {
      patchbay.stop();
    }

    @Override
    public boolean isRunning() {
      return patchbay.isRunning();
    }

    @Override
    public int activateModule(String module) throws RemoteException {
      return patchbay.activateModule(module);
    }

    @Override
    public int deactivateModule(String module) throws RemoteException {
      return patchbay.deactivateModule(module);
    }

    @Override
    public boolean isActive(String module) throws RemoteException {
      return patchbay.isActive(module);
    }

    @Override
    public int getProtocolVersion() throws RemoteException {
      return patchbay.getProtocolVersion();
    }

    @Override
    public void startForeground(int id, Notification notification) throws RemoteException {
      PatchbayService.this.startForeground(id, notification);
    }

    @Override
    public void stopForeground(boolean removeNotification) throws RemoteException {
      PatchbayService.this.stopForeground(removeNotification);
    }
  };

  @Override
  public void onCreate() {
    super.onCreate();
    try {
      patchbay = new Patchbay(this, 1, 2);
    } catch (IOException e) {
      patchbay = null;
    }
  };

  @Override
  public IBinder onBind(Intent intent) {
    return (patchbay != null) ? binder : null;
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    release();
  }

  private void release() {
    if (patchbay != null) {
      patchbay.release();
      patchbay = null;
    }
  }
}
