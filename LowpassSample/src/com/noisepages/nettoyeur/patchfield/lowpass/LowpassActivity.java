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
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.noisepages.nettoyeur.patchfield.PatchfieldActivity;

public class LowpassActivity extends PatchfieldActivity implements OnSeekBarChangeListener {

  private static final String TAG = "LowpassSample";

  private LowpassModule module = null;

  private TextView textView;

  private final String moduleLabel = "lowpass";

  private int sampleRate;

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
  }

  @Override
  protected void onDestroy() {
    if (patchfield != null) {
      try {
        module.release(patchfield);
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

  @Override
  protected void onPatchfieldConnected() {
    Log.i(TAG, "Service connected.");
    PendingIntent pi =
        PendingIntent.getActivity(LowpassActivity.this, 0, new Intent(LowpassActivity.this,
            LowpassActivity.class), 0);
    Notification notification =
        new Notification.Builder(LowpassActivity.this).setSmallIcon(R.drawable.emo_im_happy)
            .setContentTitle("LowpassModule").setContentIntent(pi).build();
    try {
      Log.i(TAG, "Creating module.");
      module = new LowpassModule(2, notification);
      module.configure(patchfield, moduleLabel);
      patchfield.activateModule(moduleLabel);
      sampleRate = patchfield.getSampleRate();
      textView.setText("Cutoff frequency: " + sampleRate + "Hz");
    } catch (RemoteException e) {
      e.printStackTrace();
    }
  }

  @Override
  protected void onPatchfieldDisconnected() {
    // Do nothing for now.
  }
}
