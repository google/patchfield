package com.noisepages.nettoyeur.patchbay.client;

import android.app.Activity;
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
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.noisepages.nettoyeur.patchbay.IPatchbayClient;
import com.noisepages.nettoyeur.patchbay.IPatchbayService;
import com.noisepages.nettoyeur.patchbay.modules.LowpassModule;

public class MainActivity extends Activity implements OnSeekBarChangeListener {

  private static final String TAG = "PatchbayClient";

  private IPatchbayService patchbay = null;

  private LowpassModule module = null;

  private final String moduleLabel = "lowpass";

  private IPatchbayClient.Stub receiver = new IPatchbayClient.Stub() {

    @Override
    public void onStart() throws RemoteException {
      Log.i(TAG, "Start!");
    }

    @Override
    public void onStop() throws RemoteException {
      Log.i(TAG, "Stop!");
    }

    @Override
    public void onModuleActivated(String module) throws RemoteException {
      Log.i(TAG, "Module activated: " + module);
    }

    @Override
    public void onModuleCreated(String module, int ins, int outs, PendingIntent intent)
        throws RemoteException {
      Log.i(TAG, "Module created: name=" + module + ", ins=" + ins + ", outs=" + outs
          + ", intent: " + intent);
    }

    @Override
    public void onModuleDeactivated(String module) throws RemoteException {
      Log.i(TAG, "Module deactivated: " + module);
    }

    @Override
    public void onModuleDeleted(String module) throws RemoteException {
      Log.i(TAG, "Module deleted: " + module);
    }

    @Override
    public void onModulesConnected(String source, int sourcePort, String sink, int sinkPort)
        throws RemoteException {
      Log.i(TAG, "Modules connected: " + source + ":" + sourcePort + ", " + sink + ":" + sinkPort);
    }

    @Override
    public void onModulesDisconnected(String source, int sourcePort, String sink, int sinkPort)
        throws RemoteException {
      Log.i(TAG, "Modules disconnected: " + source + ":" + sourcePort + ", " + sink + ":"
          + sinkPort);
    }

  };

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchbay = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchbay = IPatchbayService.Stub.asInterface(service);
      try {
        patchbay.registerClient(receiver);
        Log.i(TAG, "Creating runner.");
        module = new LowpassModule(2, null);
        module.configure(patchbay, moduleLabel);
        patchbay.activateModule(moduleLabel);
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    SeekBar cutoffBar = (SeekBar) findViewById(R.id.cutoffBar);
    cutoffBar.setOnSeekBarChangeListener(this);
    cutoffBar.setProgress(100);
    bindService(new Intent("IPatchbayService"), connection, Context.BIND_AUTO_CREATE);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    if (patchbay != null) {
      try {
        module.release(patchbay);
      } catch (RemoteException e) {
        e.printStackTrace();
      }
      try {
        patchbay.unregisterClient(receiver);
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
      module.setCutoff(q * q * q * q);
    }
  }

  @Override
  public void onStartTrackingTouch(SeekBar seekBar) {}

  @Override
  public void onStopTrackingTouch(SeekBar seekBar) {}
}
