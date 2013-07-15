package com.noisepages.nettoyeur.patchbay.stresstest;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;

import com.noisepages.nettoyeur.patchbay.AudioModule;
import com.noisepages.nettoyeur.patchbay.IPatchbayService;
import com.noisepages.nettoyeur.patchbay.samples.IdentityModule;

public class MainActivity extends Activity {

  private static final String TAG = "PatchbayStresstest";

  private final List<AudioModule> modules = new ArrayList<AudioModule>();

  private IPatchbayService patchbay = null;

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchbay = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchbay = IPatchbayService.Stub.asInterface(service);
      AudioModule prev = null;
      for (int i = 0; i < 8; ++i) {
        AudioModule module = new IdentityModule(null);
        try {
          if (module.configure(patchbay, "identity_" + i) >= 0) {
            modules.add(module);
            patchbay.activateModule(module.getName());
            if (prev != null) {
              patchbay.connectModules(prev.getName(), 0, module.getName(), 0);
            }
            prev = module;
          }
        } catch (RemoteException e) {
          e.printStackTrace();
        }
      }
      if (prev != null) {
        try {
          patchbay.connectModules(prev.getName(), 0, "system_out", 0);
          patchbay.connectModules(prev.getName(), 0, "system_out", 1);
        } catch (RemoteException e) {
          e.printStackTrace();
        }
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    bindService(new Intent("IPatchbayService"), connection, Context.BIND_AUTO_CREATE);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    if (patchbay != null) {
      try {
        for (AudioModule module : modules) {
          module.release(patchbay);
        }
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
    unbindService(connection);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }
}
