package com.noisepages.nettoyeur.patchbay.java.test;

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

import com.noisepages.nettoyeur.patchbay.IPatchbayService;
import com.noisepages.nettoyeur.patchbay.modules.JavaModule;

public class MainActivity extends Activity {

  private static final String TAG = "PatchbayJavaTest";

  private IPatchbayService patchbay = null;

  private JavaModule module = null;

  private final String moduleLabel = "java";

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchbay = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchbay = IPatchbayService.Stub.asInterface(service);
      try {
        Log.i(TAG, "Creating runner.");
        module = new JavaModule(2, 2, null) {
          @Override
          protected void process(int sampleRate, int bufferSize, int inputChannels,
              float[] inputBuffer, int outputChannels, float[] outputBuffer) {
            // Switch channels.
            System.arraycopy(inputBuffer, 0, outputBuffer, bufferSize, bufferSize);
            System.arraycopy(inputBuffer, bufferSize, outputBuffer, 0, bufferSize);
          }
        };
        module.configure(patchbay, moduleLabel);
        patchbay.activateModule(moduleLabel);
      } catch (RemoteException e) {
        e.printStackTrace();
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
        module.release(patchbay);
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
