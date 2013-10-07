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

package com.noisepages.nettoyeur.patchfield.messagesample;

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
import android.view.Menu;

import com.noisepages.nettoyeur.patchfield.IPatchfieldService;

public class MessageActivity extends Activity {

  private IPatchfieldService patchfield = null;
  private MessageModule module = null;
  private final String moduleName = "source";

  private ServiceConnection connection = new ServiceConnection() {

    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchfield = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      patchfield = IPatchfieldService.Stub.asInterface(service);
      PendingIntent pi =
          PendingIntent.getActivity(MessageActivity.this, 0, new Intent(MessageActivity.this,
              MessageActivity.class), 0);
      Notification notification =
          new Notification.Builder(MessageActivity.this).setSmallIcon(R.drawable.perm_group_audio_settings)
              .setContentTitle("MessageModule").setContentIntent(pi).build();
      module = new MessageModule(notification);
      try {
        module.configure(patchfield, moduleName);
        patchfield.activateModule(moduleName);
        patchfield.receiveMessages(8000);
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_message);
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
    getMenuInflater().inflate(R.menu.message, menu);
    return true;
  }
}
