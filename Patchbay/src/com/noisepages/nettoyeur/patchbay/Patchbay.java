package com.noisepages.nettoyeur.patchbay;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import android.app.PendingIntent;
import android.content.Context;
import android.os.IBinder;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.util.Log;

/**
 * The Java part of the Patchbay service implementation. This is mostly
 * boilerplate; the action is in the native code, Patchbay/jni/patchbay.c.
 */
public class Patchbay implements IPatchbayService {
	
	private static final String TAG = "Patchbay";

	static {
		System.loadLibrary("patchbay");
	}
	
	private final OpenSlParams params;
	private long streamPtr;
	private final Map<String, Integer> modules = new LinkedHashMap<String, Integer>();
	private final Map<String,PendingIntent> intents = new LinkedHashMap<String, PendingIntent>();
	private final RemoteCallbackList<IPatchbayClient> clients =
			new RemoteCallbackList<IPatchbayClient>();
	
	public Patchbay(Context context, int inputChannels, int outputChannels) throws IOException {
		params = OpenSlParams.createInstance(context);
		streamPtr = createInstance(params.getSampleRate(), params.getBufferSize(),
				inputChannels, outputChannels);
		if (streamPtr == 0) {
			throw new IOException("Unable to open opensl_stream.");
		}
		Log.i(TAG, "Created stream with ptr " + streamPtr);
		modules.put("system_in", 0);
		modules.put("system_out", 1);
	}
	
	public synchronized void release() {
		if (streamPtr != 0) {
			releaseInstance(streamPtr);
			streamPtr = 0;
			clients.kill();
		}
	}

	@Override
	public synchronized void registerClient(IPatchbayClient client) throws RemoteException {
		clients.register(client);
	}

	@Override
	public synchronized void unregisterClient(IPatchbayClient client) throws RemoteException {
		clients.unregister(client);
	}

	@Override
	public synchronized int getSampleRate() {
		return params.getSampleRate();
	}

	@Override
	public synchronized int getBufferSize() {
		return params.getBufferSize();
	}

	@Override
	public synchronized int sendSharedMemoryFileDescriptor() {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		return PatchbayException.successOrFailure(sendSharedMemoryFileDescriptor(streamPtr));
	}
	
	@Override
	public synchronized int start() {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		int result = start(streamPtr);
		if (result == 0) {
			int i = clients.beginBroadcast();
			while (--i >= 0) {
				try {
					clients.getBroadcastItem(i).onStart();
				} catch (RemoteException e) {
					// Do nothing; RemoteCallbackList will take care of the cleanup.
				}
			}
			clients.finishBroadcast();
		}
		return PatchbayException.successOrFailure(result);
	}

	@Override
	public synchronized void stop() {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		stop(streamPtr);
		int i = clients.beginBroadcast();
		while (--i >= 0) {
			try {
				clients.getBroadcastItem(i).onStop();
			} catch (RemoteException e) {
				// Do nothing; RemoteCallbackList will take care of the cleanup.
			}
		}
		clients.finishBroadcast();
	}

	@Override
	public synchronized boolean isRunning() {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		return isRunning(streamPtr);
	}

	@Override
	public synchronized int createModule(String module, int inputChannels, int outputChannels, PendingIntent intent) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		if (inputChannels < 0 || outputChannels < 0 || (inputChannels == 0 && outputChannels == 0)) {
			return PatchbayException.INVALID_PARAMETERS;
		}
		if (modules.containsKey(module)) {
			return PatchbayException.MODULE_NAME_TAKEN;
		}
		int index = createModule(streamPtr, inputChannels, outputChannels);
		if (index >= 0) {
			modules.put(module, index);
			intents.put(module, intent);
			int i = clients.beginBroadcast();
			while (--i >= 0) {
				try {
					clients.getBroadcastItem(i).onModuleCreated(module, inputChannels, outputChannels);
				} catch (RemoteException e) {
					// Do nothing; RemoteCallbackList will take care of the cleanup.
				}
			}
			clients.finishBroadcast();
		}
		return index;
	}

	@Override
	public synchronized int deleteModule(String module) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		if (!modules.containsKey(module)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		int result = deleteModule(streamPtr, modules.get(module));
		if (result == 0) {
			modules.remove(module);
			intents.remove(module);
			int i = clients.beginBroadcast();
			while (--i >= 0) {
				try {
					clients.getBroadcastItem(i).onModuleDeleted(module);
				} catch (RemoteException e) {
					// Do nothing; RemoteCallbackList will take care of the cleanup.
				}
			}
			clients.finishBroadcast();
		}
		return result;
	}

	@Override
	public synchronized int connectModules(String source, int sourcePort, String sink, int sinkPort) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		if (!modules.containsKey(source)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		if (!modules.containsKey(sink)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		if (sourcePort < 0 || sourcePort >= getOutputChannels(source)) {
			return PatchbayException.PORT_OUT_OF_RANGE;
		}
		if (sinkPort < 0 || sinkPort >= getInputChannels(sink)) {
			return PatchbayException.PORT_OUT_OF_RANGE;
		}
		if (isConnected(source, sourcePort, sink, sinkPort)) {
			return PatchbayException.SUCCESS;
		}
		if (isDependent(source, sink)) {
			return PatchbayException.CYCLIC_DEPENDENCY;
		}
		int result = connectModules(streamPtr, modules.get(source), sourcePort, modules.get(sink), sinkPort);
		if (result == 0) {
			int i = clients.beginBroadcast();
			while (--i >= 0) {
				try {
					clients.getBroadcastItem(i).onModulesConnected(source, sourcePort, sink, sinkPort);
				} catch (RemoteException e) {
					// Do nothing; RemoteCallbackList will take care of the cleanup.
				}
			}
			clients.finishBroadcast();
		}
		return result;
	}

	@Override
	public synchronized int disconnectModules(String source, int sourcePort, String sink, int sinkPort) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		if (!modules.containsKey(source)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		if (!modules.containsKey(sink)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		if (sourcePort < 0 || sourcePort >= getOutputChannels(source)) {
			return PatchbayException.PORT_OUT_OF_RANGE;
		}
		if (sinkPort < 0 || sinkPort >= getInputChannels(sink)) {
			return PatchbayException.PORT_OUT_OF_RANGE;
		}
		if (!isConnected(source, sourcePort, sink, sinkPort)) {
			return 0;
		}
		int result = disconnectModules(streamPtr, modules.get(source), sourcePort, modules.get(sink), sinkPort);
		if (result == 0) {
			int i = clients.beginBroadcast();
			while (--i >= 0) {
				try {
					clients.getBroadcastItem(i).onModulesDisconnected(source, sourcePort, sink, sinkPort);
				} catch (RemoteException e) {
					// Do nothing; RemoteCallbackList will take care of the cleanup.
				}
			}
			clients.finishBroadcast();
		}
		return result;
	}

	@Override
	public synchronized boolean isConnected(String source, int sourcePort, String sink,
			int sinkPort) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		return modules.containsKey(source) && modules.containsKey(sink) &&
				isConnected(streamPtr, modules.get(source), sourcePort, modules.get(sink), sinkPort);
	}

	@Override
	public synchronized boolean isDependent(String sink, String source) {
		List<String> dependents = getDependents(source);
		return dependents.contains(sink);
	}

	@Override
	public synchronized List<String> getModules() {
		return Collections.unmodifiableList(new ArrayList<String>(modules.keySet()));
	}

	@Override
	public synchronized int getInputChannels(String module) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		if (!modules.containsKey(module)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		return getInputChannels(streamPtr, modules.get(module));
	}

	@Override
	public synchronized int getOutputChannels(String module) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		if (!modules.containsKey(module)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		return getOutputChannels(streamPtr, modules.get(module));
	}

	@Override
	public synchronized PendingIntent getIntent(String module) {
		return intents.get(module);
	}

	@Override
	public synchronized boolean isActive(String module) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		return modules.containsKey(module) && isActive(streamPtr, modules.get(module));
	}

	@Override
	public synchronized int activateModule(String module) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		if (!modules.containsKey(module)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		if (isActive(module)) {
			return 0;
		}
		int result = activateModule(streamPtr, modules.get(module));
		if (result == 0) {
			int i = clients.beginBroadcast();
			while (--i >= 0) {
				try {
					clients.getBroadcastItem(i).onModuleActivated(module);
				} catch (RemoteException e) {
					// Do nothing; RemoteCallbackList will take care of the cleanup.
				}
			}
			clients.finishBroadcast();
		}
		return PatchbayException.successOrFailure(result);
	}

	@Override
	public synchronized int deactivateModule(String module) {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		if (!modules.containsKey(module)) {
			return PatchbayException.NO_SUCH_MODULE;
		}
		if (!isActive(module)) {
			return 0;
		}
		int result = deactivateModule(streamPtr, modules.get(module));
		if (result == 0) {
			int i = clients.beginBroadcast();
			while (--i >= 0) {
				try {
					clients.getBroadcastItem(i).onModuleDeactivated(module);
				} catch (RemoteException e) {
					// Do nothing; RemoteCallbackList will take care of the cleanup.
				}
			}
			clients.finishBroadcast();
		}
		return PatchbayException.successOrFailure(result);
	}

	@Override
	public int getProtocolVersion() throws RemoteException {
		if (streamPtr == 0) {
			throw new IllegalStateException("Stream closed.");
		}
		return getProtocolVersion(streamPtr);
	}
	
	private native long createInstance(int sampleRate, int bufferSize,
			int inputChannels, int outputChannels);
	private native int sendSharedMemoryFileDescriptor(long streamPtr);
	private native void releaseInstance(long streamPtr);
	private native int createModule(long streamPtr, int inputChannels, int outputChannels);
	private native int deleteModule(long streamPtr, int index);
	private native int connectModules(
			long streamPtr, int sourceIndex, int sourcePort, int sinkIndex, int sinkPort);
	private native int disconnectModules(
			long streamPtr, int sourceIndex, int sourcePort, int sinkIndex, int sinkPort);
	private native int activateModule(long streamPtr, int index);
	private native int deactivateModule(long streamPtr, int index);
	private native int start(long streamPtr);
	private native void stop(long streamPtr);
	private native boolean isActive(long streamPtr, int index);
	private native boolean isRunning(long streamPtr);
	private native boolean isConnected(long streamPtr, int sourceIndex, int sourcePort,
			int sinkIndex, int sinkPort);
	private native int getInputChannels(long streamPtr, int index);
	private native int getOutputChannels(long streamPtr, int index);
	private native int getProtocolVersion(long streamPtr);

	@Override
	public IBinder asBinder() {
		throw new UnsupportedOperationException("Not implemented for local patchbay.");
	}

	private List<String> getDependents(String source) {
		List<String> dependents = new ArrayList<String>();
		collectDependents(source, dependents);
		return dependents;
	}

	private void collectDependents(String source, List<String> dependents) {
		dependents.add(source);
		for (String sink : modules.keySet()) {
			if (!dependents.contains(sink) && isDirectDependent(sink, source)) {
				collectDependents(sink, dependents);
			}
		}
	}

	private boolean isDirectDependent(String sink, String source) {
		for (int i = 0; i < getOutputChannels(source); ++i) {
			for (int j = 0; j < getInputChannels(sink); ++j) {
				if (isConnected(source, i, sink, j)) {
					return true;
				}
			}
		}
		return false;
	}
}
