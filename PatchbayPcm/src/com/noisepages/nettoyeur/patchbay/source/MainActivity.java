package com.noisepages.nettoyeur.patchbay.source;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.view.Menu;
import android.widget.TextView;

import com.noisepages.nettoyeur.patchbay.IPatchbayService;

public class MainActivity extends Activity {

  @SuppressWarnings("unused")
  private static final String TAG = "PatchbayPcm";

  private TextView mainText = null;
  private IPatchbayService patchbay = null;
  private PcmSource source = null;
  private final String moduleName = "source";

  private ServiceConnection connection = new ServiceConnection() {

    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchbay = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      patchbay = IPatchbayService.Stub.asInterface(service);
      InputStream is = getResources().openRawResource(R.raw.rst);
      ByteBuffer buffer;
      try {
        buffer = ByteBuffer.allocateDirect(is.available());
        is.read(buffer.array());
      } catch (IOException e) {
        e.printStackTrace();
        return;
      }
      source = new PcmSource(2, buffer, null);
      mainText.setText("Relaxation Spa Treatment");
      try {
        source.configure(patchbay, moduleName);
        patchbay.activateModule(moduleName);
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    mainText = (TextView) findViewById(R.id.mainText);
    bindService(new Intent("IPatchbayService"), connection, Context.BIND_AUTO_CREATE);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    if (patchbay != null) {
      try {
        source.release(patchbay);
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
