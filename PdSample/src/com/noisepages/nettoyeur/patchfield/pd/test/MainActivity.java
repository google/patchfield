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

package com.noisepages.nettoyeur.patchfield.pd.test;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

import org.puredata.core.PdBase;
import org.puredata.core.utils.IoUtils;
import org.puredata.core.utils.PdDispatcher;

import android.app.Activity;
import android.app.Notification;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;

import com.noisepages.nettoyeur.patchfield.IPatchFieldService;
import com.noisepages.nettoyeur.patchfield.pd.PdModule;

public class MainActivity extends Activity {

  private static final String TAG = "PatchFieldPdSample";

  private IPatchFieldService patchfield = null;

  private PdModule module = null;
  private final String label = "pdtest";

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchfield = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchfield = IPatchFieldService.Stub.asInterface(service);
      int inputChannels = 2;
      int outputChannels = 2;
      Notification notification = new Notification.Builder(MainActivity.this)
          .setSmallIcon(R.drawable.pd_icon)
          .setContentTitle("PdModule")
          .build();
      try {
        // Create PdModule instance before invoking any methods on PdBase.
        module =
            PdModule.getInstance(patchfield.getSampleRate(), inputChannels, outputChannels, notification);
        PdBase.setReceiver(new PdDispatcher() {
          @Override
          public void print(String s) {
            Log.i(TAG, s);
          }
        });
        InputStream in = getResources().openRawResource(R.raw.test);
        File pdFile = IoUtils.extractResource(in, "test.pd", getCacheDir());
        PdBase.openPatch(pdFile);
        module.configure(patchfield, label);
        patchfield.activateModule(label);
      } catch (RemoteException e) {
        e.printStackTrace();
      } catch (IOException e) {
        e.printStackTrace();
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    bindService(new Intent("IPatchFieldService"), connection, Context.BIND_AUTO_CREATE);
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
    PdBase.release();
    unbindService(connection);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }

}
