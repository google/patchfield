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

package com.noisepages.nettoyeur.patchfield.source;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import com.noisepages.nettoyeur.patchfield.PatchfieldActivity;

public class PcmActivity extends PatchfieldActivity {

  private static final String TAG = "PatchfieldPcmSample";

  private PcmSource source = null;
  private final String moduleName = "source";

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    Button button = (Button) findViewById(R.id.connectButton);
    button.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        if (patchfield != null) {
          try {
            patchfield.connectPorts(moduleName, 0, "system_out", 0);
            patchfield.connectPorts(moduleName, 1, "system_out", 1);
          } catch (RemoteException e) {
            e.printStackTrace();
          }
        }
      }
    });
  }

  @Override
  protected void onDestroy() {
    if (patchfield != null) {
      try {
        source.release(patchfield);
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
    super.onDestroy();
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }

  @Override
  protected void onPatchfieldConnected() {
    int srate = 44100;
    try {
      srate = patchfield.getSampleRate();
    } catch (RemoteException e) {
      e.printStackTrace();
    }
    Log.i(TAG, "Loading resource for sample rate " + srate + ".");
    // Note that the included wav resources are _headless_ wav files,
    // i.e., they contain only audio, no metadata.
    InputStream is =
        getResources().openRawResource(srate == 44100 ? R.raw.rst44100 : R.raw.rst48000);
    ByteBuffer buffer;
    try {
      buffer = ByteBuffer.allocateDirect(is.available());
      is.read(buffer.array());
    } catch (IOException e) {
      e.printStackTrace();
      return;
    }
    PendingIntent pi =
        PendingIntent.getActivity(PcmActivity.this, 0, new Intent(PcmActivity.this,
            PcmActivity.class), 0);
    Notification notification =
        new Notification.Builder(PcmActivity.this).setSmallIcon(R.drawable.perm_group_voicemail)
            .setContentTitle("Relaxation Spa Treatment").setContentIntent(pi).build();
    source = new PcmSource(2, buffer, notification);
    try {
      source.configure(patchfield, moduleName);
      patchfield.activateModule(moduleName);
    } catch (RemoteException e) {
      e.printStackTrace();
    }
  }

  @Override
  protected void onPatchfieldDisconnected() {
    // Do nothing for now.
  }
}
