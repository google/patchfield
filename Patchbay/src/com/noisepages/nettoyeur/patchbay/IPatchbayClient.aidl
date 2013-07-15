package com.noisepages.nettoyeur.patchbay;

import android.app.PendingIntent;

/**
 * Patchbay client interface for handling notifications when the state of the
 * patchbay changes.
 */
oneway interface IPatchbayClient {

  void onModuleCreated(String name, int inputChannels, int outputChannels, in PendingIntent intent);
  void onModuleDeleted(String name);
  
  void onModuleActivated(String name);
  void onModuleDeactivated(String name);
    
  void onModulesConnected(String source, int sourcePort, String sink, int sinkPort);
  void onModulesDisconnected(String source, int sourcePort, String sink, int sinkPort);
    
  void onStart();
  void onStop();
}
