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

package com.noisepages.nettoyeur.patchfield.java.test;

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

import com.noisepages.nettoyeur.patchfield.IPatchfieldService;
import com.noisepages.nettoyeur.patchfield.modules.JavaModule;

public class MainActivity extends Activity {

  private static final String TAG = "JavaSample";

  private IPatchfieldService patchfield = null;

  private JavaModule module = null;

  private final String moduleLabel = "java";

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchfield = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchfield = IPatchfieldService.Stub.asInterface(service);
      try {
        Log.i(TAG, "Creating runner.");
        module = new JavaModule(64, 2, 2, null) {
          @Override
          protected void process(int sampleRate, int bufferSize, int inputChannels,
              float[] inputBuffer, int outputChannels, float[] outputBuffer) {
            // Switch channels.
            System.arraycopy(inputBuffer, 0, outputBuffer, bufferSize, bufferSize);
            System.arraycopy(inputBuffer, bufferSize, outputBuffer, 0, bufferSize);
          }
        };
        module.configure(patchfield, moduleLabel);
        patchfield.activateModule(moduleLabel);
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    bindService(new Intent("IPatchfieldService"), connection, Context.BIND_AUTO_CREATE);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    if (patchfield != null) {
      try {
        module.release(patchfield);
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
