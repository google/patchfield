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

package com.noisepages.nettoyeur.patchfield.lowpass;

import android.app.Activity;
import android.app.Notification;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.noisepages.nettoyeur.patchfield.IPatchFieldService;

public class MainActivity extends Activity implements OnSeekBarChangeListener {

  private static final String TAG = "PatchFieldLowpassSample";

  private IPatchFieldService patchfield = null;

  private LowpassModule module = null;

  private TextView textView;

  private final String moduleLabel = "lowpass";
  
  private int sampleRate;

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchfield = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchfield = IPatchFieldService.Stub.asInterface(service);
      PendingIntent pi =
          PendingIntent.getActivity(MainActivity.this, 0, new Intent(MainActivity.this,
              MainActivity.class), 0);
      Notification notification =
          new Notification.Builder(MainActivity.this).setSmallIcon(R.drawable.emo_im_happy)
              .setContentTitle("LowpassModule").setContentIntent(pi).build();
      try {
        Log.i(TAG, "Creating runner.");
        module = new LowpassModule(2, notification);
        module.configure(patchfield, moduleLabel);
        patchfield.activateModule(moduleLabel);
        sampleRate = patchfield.getSampleRate();
        textView.setText("Cutoff frequency: " + sampleRate + "Hz");
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    textView = (TextView) findViewById(R.id.mainText);
    SeekBar cutoffBar = (SeekBar) findViewById(R.id.cutoffBar);
    cutoffBar.setOnSeekBarChangeListener(this);
    cutoffBar.setProgress(100);
    Button button = (Button) findViewById(R.id.connectButton);
    button.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        if (patchfield != null) {
          try {
            patchfield.connectPorts(moduleLabel, 0, "system_out", 0);
            patchfield.connectPorts(moduleLabel, 1, "system_out", 1);
          } catch (RemoteException e) {
            e.printStackTrace();
          }
        }
      }
    });
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
    unbindService(connection);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }

  @Override
  public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
    if (module != null) {
      double q = progress * 0.01;
      q = q * q * q * q * q;
      textView.setText("Cutoff frequency: " + (int) (q * sampleRate) + "Hz");
      module.setCutoff(q);
    }
  }

  @Override
  public void onStartTrackingTouch(SeekBar seekBar) {}

  @Override
  public void onStopTrackingTouch(SeekBar seekBar) {}
}
