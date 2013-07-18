package com.noisepages.nettoyeur.patchbay.control;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.app.Notification;
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
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.noisepages.nettoyeur.patchbay.IPatchbayClient;
import com.noisepages.nettoyeur.patchbay.IPatchbayService;

public class MainActivity extends Activity implements OnCheckedChangeListener {

  private static final String TAG = "PatchControl";

  private IPatchbayService patchbay = null;

  private TextView displayLine;
  private ToggleButton playButton;
  private PatchView patchView;
  private ViewGroup iconView;
  private Map<String, View> moduleViews = new HashMap<String, View>();

  private IPatchbayClient.Stub receiver = new IPatchbayClient.Stub() {

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
    public void onModuleActivated(String arg0) throws RemoteException {}

    @Override
    public void onModuleCreated(final String module, final int inputs, final int outputs,
        final Notification notification) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.addModule(module, inputs, outputs, notification);
          if (notification != null) {
            View view = notification.contentView.apply(MainActivity.this, iconView);
            moduleViews.put(module, view);
            iconView.addView(view);
          }
        }
      });
    }

    @Override
    public void onModuleDeactivated(String module) throws RemoteException {}

    @Override
    public void onModuleDeleted(final String module) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.deleteModule(module);
          View view = moduleViews.remove(module);
          if (view != null) {
            iconView.removeView(view);
          }
        }
      });
    }

    @Override
    public void onModulesConnected(final String source, final int sourcePort, final String sink,
        final int sinkPort) throws RemoteException {
      runOnUiThread(new Runnable() {
        @Override
        public void run() {
          patchView.addConnection(source, sourcePort, sink, sinkPort);
        }
      });
    }

    @Override
    public void onModulesDisconnected(final String source, final int sourcePort, final String sink,
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
      patchbay = null;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
      Log.i(TAG, "Service connected.");
      patchbay = IPatchbayService.Stub.asInterface(service);
      patchView.setPatchbay(patchbay);
      try {
        patchbay.registerClient(receiver);
        playButton.setChecked(patchbay.isRunning());
        displayLine.setText("Sample rate: " + patchbay.getSampleRate() + ", buffer size: "
            + patchbay.getBufferSize() + ", protocol version: " + patchbay.getProtocolVersion());
        List<String> modules = patchbay.getModules();
        for (String module : modules) {
          patchView.addModule(module, patchbay.getInputChannels(module),
              patchbay.getOutputChannels(module), patchbay.getNotification(module));
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
    playButton = (ToggleButton) findViewById(R.id.playButton);
    playButton.setOnCheckedChangeListener(this);
    patchView = (PatchView) findViewById(R.id.patchView);
    iconView = (ViewGroup) findViewById(R.id.iconView);
    bindService(new Intent("IPatchbayService"), connection, Context.BIND_AUTO_CREATE);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    if (patchbay != null) {
      try {
        patchbay.unregisterClient(receiver);
      } catch (RemoteException e) {
        e.printStackTrace();
      }
    }
    unbindService(connection);
    patchbay = null;
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }

  @Override
  public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
    if (patchbay == null) {
      return;
    }
    try {
      if (isChecked) {
        patchbay.start();
      } else {
        patchbay.stop();
      }
    } catch (RemoteException e) {
      e.printStackTrace();
    }
  }
}
