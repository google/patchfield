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

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;

import com.noisepages.nettoyeur.patchfield.PatchfieldActivity;
import com.noisepages.nettoyeur.patchfield.PatchfieldException;

public class MessageActivity extends PatchfieldActivity {

  private MessageModule module = null;
  private final String moduleName = "message";

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_message);
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
    getMenuInflater().inflate(R.menu.message, menu);
    return true;
  }

  @Override
  protected void onPatchfieldConnected() {
    PendingIntent pi =
        PendingIntent.getActivity(MessageActivity.this, 0, new Intent(MessageActivity.this,
            MessageActivity.class), 0);
    Notification notification =
        new Notification.Builder(MessageActivity.this)
            .setSmallIcon(R.drawable.perm_group_audio_settings).setContentTitle("MessageModule")
            .setContentIntent(pi).build();
    module = new MessageModule(notification);
    try {
      PatchfieldException.throwOnError(module.configure(patchfield, moduleName));
      patchfield.activateModule(moduleName);
      patchfield.receiveMessages(8000);
    } catch (RemoteException e) {
      e.printStackTrace();
    } catch (PatchfieldException e) {
      Log.e(getClass().getName(), "Error code: " + e.getCode());
      e.printStackTrace();
      finish();
    }
  }

  @Override
  protected void onPatchfieldDisconnected() {
    // Do nothing for now.
  }
}
