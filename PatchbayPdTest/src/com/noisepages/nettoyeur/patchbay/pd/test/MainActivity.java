package com.noisepages.nettoyeur.patchbay.pd.test;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

import org.puredata.core.PdBase;
import org.puredata.core.utils.IoUtils;
import org.puredata.core.utils.PdDispatcher;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.Menu;

import com.noisepages.nettoyeur.patchbay.IPatchbayService;
import com.noisepages.nettoyeur.patchbay.pd.PdModule;

public class MainActivity extends Activity {

	private static final String TAG = "PatchbayPdTest";
	
	private IPatchbayService patchbay = null;

	private PdModule module = null;
	
	private ServiceConnection connection = new ServiceConnection() {
		@Override
		public void onServiceDisconnected(ComponentName name) {
			patchbay = null;
		}

		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			Log.i(TAG, "Service connected.");
			patchbay = IPatchbayService.Stub.asInterface(service);
			int inputChannels = 2;
			int outputChannels = 2;
			module = PdModule.getInstance(inputChannels, outputChannels, null);
			PdBase.setReceiver(new PdDispatcher() {
				@Override
				public void print(String s) {
					Log.i(TAG, s);
				}
			});
			try {
				PdBase.openAudio(inputChannels, outputChannels, patchbay.getSampleRate());
				PdBase.closeAudio();  // Shut down internal OpenSL instance; we don't need it here.
				PdBase.computeAudio(true);
				InputStream in = getResources().openRawResource(R.raw.test);
				File patchFile = IoUtils.extractResource(in, "test.pd", getCacheDir());
				PdBase.openPatch(patchFile);
			} catch (IOException e) {
				e.printStackTrace();
			} catch (RemoteException e) {
				e.printStackTrace();
			}
			try {
				module.configure(patchbay, "PdTest");
				patchbay.activateModule("PdTest");
			} catch (RemoteException e) {
				e.printStackTrace();
			}
		}
	};
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		bindService(new Intent("IPatchbayService"),
				connection, Context.BIND_AUTO_CREATE);
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
		PdBase.release();
		unbindService(connection);
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

}
