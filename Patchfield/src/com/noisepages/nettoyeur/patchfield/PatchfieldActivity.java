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

package com.noisepages.nettoyeur.patchfield;

import android.app.Activity;
import android.app.Notification;
import android.app.PendingIntent.CanceledException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.view.KeyEvent;

/**
 * This convenience class encapsulates two common features of activities that use Patchfield, the
 * connection to the Patchfield service and navigation to a control activity via a long press on
 * the back button.
 */
public abstract class PatchfieldActivity extends Activity {

  protected IPatchfieldService patchfield = null;

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchfield = null;
      onPatchfieldDisconnected();
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      patchfield = IPatchfieldService.Stub.asInterface(service);
      onPatchfieldConnected();
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    bindService(new Intent("IPatchfieldService"), connection, Context.BIND_AUTO_CREATE);
  }

  protected abstract void onPatchfieldConnected();

  protected abstract void onPatchfieldDisconnected();

  /**
   * When overriding the onDestroy method, make sure to invoke this parent method _at the end_.
   */
  @Override
  protected void onDestroy() {
    unbindService(connection);
    patchfield = null;
    super.onDestroy();
  }
  
  @Override
  public boolean onKeyLongPress(int keyCode, KeyEvent event) {
    if (keyCode == KeyEvent.KEYCODE_BACK && patchfield != null) {
      try {
        Notification notification = patchfield.getNotification(null);
        if (notification != null) {
          notification.contentIntent.send();
          return true;
        }
      } catch (RemoteException e) {
        // Do nothing.
      } catch (CanceledException e) {
        // Do nothing.
      }
    }
    return super.onKeyLongPress(keyCode, event);
  }
}
