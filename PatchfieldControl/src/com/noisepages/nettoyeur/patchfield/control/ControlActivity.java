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

package com.noisepages.nettoyeur.patchfield.control;

import java.util.List;

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
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.FrameLayout;
import android.widget.Switch;
import android.widget.TextView;

import com.noisepages.nettoyeur.patchfield.IPatchfieldClient;
import com.noisepages.nettoyeur.patchfield.IPatchfieldService;

public class ControlActivity extends Activity implements OnCheckedChangeListener {

  private static final String TAG = "PatchControl";

  private IPatchfieldService patchfield = null;

  private TextView displayLine;
  private Switch playButton;
  private PatchView patchView;

  private IPatchfieldClient.Stub receiver = new IPatchfieldClient.Stub() {

    @Override
    public void onStart() throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          playButton.setChecked(true);
        }
      });
    }

    @Override
    public void onStop() throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          playButton.setChecked(false);
        }
      });
    }

    @Override
    public void onModuleCreated(final String module, final int inputs, final int outputs,
        final Notification notification) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.addModule(module, inputs, outputs, notification);
        }
      });
    }

    @Override
    public void onModuleActivated(final String module) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.activateModule(module);
        }
      });
    }

    @Override
    public void onModuleDeactivated(final String module) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.deactivateModule(module);
        }
      });
    }

    @Override
    public void onModuleDeleted(final String module) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.deleteModule(module);
        }
      });
    }

    @Override
    public void onPortsConnected(final String source, final int sourcePort, final String sink,
        final int sinkPort) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.addConnection(source, sourcePort, sink, sinkPort);
        }
      });
    }

    @Override
    public void onPortsDisconnected(final String source, final int sourcePort, final String sink,
        final int sinkPort) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.removeConnection(source, sourcePort, sink, sinkPort);
        }
      });
    }
  };

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchfield = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchfield = IPatchfieldService.Stub.asInterface(service);
      patchView.setPatchfield(patchfield);
      PendingIntent pi =
          PendingIntent.getActivity(ControlActivity.this, 0, new Intent(ControlActivity.this,
              ControlActivity.class), 0);
      Notification notification =
          new Notification.Builder(ControlActivity.this)
              .setSmallIcon(android.R.drawable.ic_media_play).setContentTitle("PatchfieldControl")
              .setContentIntent(pi).build();
      try {
        patchfield.startForeground(1, notification);
        patchfield.registerClient(receiver);
        playButton.setChecked(patchfield.isRunning());
        displayLine.setText("Sample rate: " + patchfield.getSampleRate() + ", buffer size: "
            + patchfield.getBufferSize() + ", protocol version: " + patchfield.getProtocolVersion());
        List<String> modules = patchfield.getModules();
        for (String module : modules) {
          patchView.addModule(module, patchfield.getInputChannels(module),
              patchfield.getOutputChannels(module), patchfield.getNotification(module));
        }
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    displayLine = (TextView) findViewById(R.id.displayLine);
    playButton = (Switch) findViewById(R.id.playButton);
    playButton.setOnCheckedChangeListener(this);
    FrameLayout frame = (FrameLayout) findViewById(R.id.moduleFrame);
    patchView = new PatchView(this);
    patchView.init(this, frame);
    bindService(new Intent("IPatchfieldService"), connection, Context.BIND_AUTO_CREATE);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    if (patchfield != null) {
      try {
        patchfield.stopForeground(false);
        patchfield.unregisterClient(receiver);
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
    unbindService(connection);
    patchfield = null;
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }

  @Override
  public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
    if (patchfield != null) {
      try {
        if (isChecked) {
          patchfield.start();
        } else {
          patchfield.stop();
        }
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
  }
}
