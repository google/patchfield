package com.noisepages.nettoyeur.patchbay.lowpass;

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
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.noisepages.nettoyeur.patchbay.IPatchbayService;
import com.noisepages.nettoyeur.patchbay.modules.LowpassModule;

public class MainActivity extends Activity implements OnSeekBarChangeListener {

  private static final String TAG = "PatchbayLowpassSample";

  private IPatchbayService patchbay = null;

  private LowpassModule module = null;

  private TextView textView;

  private final String moduleLabel = "lowpass";
  
  private int sampleRate;

  private ServiceConnection connection = new ServiceConnection() {
    @Override
    public void onServiceDisconnected(ComponentName name) {
      patchbay = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchbay = IPatchbayService.Stub.asInterface(service);
      PendingIntent pi =
          PendingIntent.getActivity(MainActivity.this, 0, new Intent(MainActivity.this,
              MainActivity.class), 0);
      Notification notification =
          new Notification.Builder(MainActivity.this).setSmallIcon(R.drawable.emo_im_happy)
              .setContentTitle("LowpassModule").setContentIntent(pi).build();
      try {
        Log.i(TAG, "Creating runner.");
        module = new LowpassModule(2, notification);
        module.configure(patchbay, moduleLabel);
        patchbay.activateModule(moduleLabel);
        sampleRate = patchbay.getSampleRate();
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
