package com.noisepages.nettoyeur.patchbay;

import android.app.Notification;

/**
 * Patchbay client interface for handling notifications when the state of the
 * patchbay changes.
 */
oneway interface IPatchbayClient {

  void onModuleCreated(String name, int inputChannels, int outputChannels, in Notification notification);
  void onModuleDeleted(String name);
  
  void onModuleActivated(String name);
  void onModuleDeactivated(String name);
    
  void onPortsConnected(String source, int sourcePort, String sink, int sinkPort);
  void onPortsDisconnected(String source, int sourcePort, String sink, int sinkPort);
    
  void onStart();
  void onStop();
}
