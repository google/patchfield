package com.noisepages.nettoyeur.patchbay;

import android.os.RemoteException;
import android.util.Log;

/**
 * <p>
 * Abstract base class for Patchbay audio modules. Subclasses must implement
 * methods for creating and releasing audio modules; these implementations will
 * involve native code using the native audio_module library in Patchbay/jni.
 * </p>
 * 
 * <p>
 * The intended usage is to call the configure method once to set up the audio
 * module, and to call release when it is done. If the module times out (due to a
 * misbehaving audio processing callback), the recommended response is to just
 * release the module (and to create a new instance if you still need its
 * functionality).
 * </p>
 * 
 * <p>
 * If, however, you want to reinstate an audio module that has timed out, you need
 * to perform the following steps (in native code):
 * </p>
 * <ol>
 * <li>Call au_release(void *p) to release the data structure that connects your
 * local audio module to its representation in the remote service.</li>
 * 
 * <li>If the processing context of your audio module is mutable, make sure it is
 * intact. (The timeout interrupts the processing callback with a real-time
 * signal and then exits the rendering loop with a nonlocal goto, i.e., chances
 * are that a timeout will leave any mutable context in an invalid state.)</li>
 * 
 * <li>Call au_create(...) again, with the same token and index as before. This
 * will reestablish the connection between your local audio module and the remote
 * service. The methods getToken and getIndex exist for this purpose.</li>
 * </ol>
 */
public abstract class AudioModule {

	private static final String TAG = "AudioModule";
	
	private String name = null;
	private int token = -1;
	private int index = -1;
	
	private class FdReceiverThread extends Thread {
		@Override
		public void run() {
			token = SharedMemoryUtils.receiveSharedMemoryFileDescriptor();
			Log.i(TAG, "fd: " + token);
		}
	}
	
	/**
	 * This method takes care of the elaborate choreography that it takes to set
	 * up an audio module and to connect it to its representation in the Patchbay
	 * service.
	 * 
	 * Specifically, it sets up the shared memory between the local module and the
	 * Patchbay service, creates a new module in the service, and connects it to
	 * the local module.
	 * 
	 * @param patchbay
	 *            stub for communicating with the Patchbay service
	 * @param name
	 *            of the new audio module in Patchbay
	 * @return 0 on success, a negative error on failure; use
	 *         {@link PatchbayException} to interpret the return value.
	 * @throws RemoteException
	 */
	public int configure(IPatchbayService patchbay, String name) throws RemoteException {
		if (this.name != null) {
			throw new IllegalStateException("Module is already configured.");
		}
		FdReceiverThread t = new FdReceiverThread();
		t.start();
		while (patchbay.sendSharedMemoryFileDescriptor() != 0 && t.isAlive()) {
			try {
				Thread.sleep(10);  // Wait for receiver thread to spin up.
			} catch (InterruptedException e) {
				Thread.currentThread().interrupt();
			}
		}
		try {
			t.join();
		} catch (InterruptedException e) {
			Thread.currentThread().interrupt();
		}
		if (token < 0) {
			return token;
		}
		index = patchbay.createModule(name, getInputChannels(), getOutputChannels());
		if (index < 0) {
			SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
			return index;
		}
		if (!configure(name, token, index)) {
			patchbay.deleteModule(name);
			SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
			return PatchbayException.FAILURE;
		}
		this.name = name;
		return PatchbayException.SUCCESS;
	}
	
	/**
	 * Releases all resources associated with this module and deletes its
	 * representation in the Patchbay service.
	 * 
	 * @param patchbay
	 *            stub for communicating with the Patchbay service
	 * @throws RemoteException
	 */
	public void release(IPatchbayService patchbay) throws RemoteException {
		if (name != null) {
			patchbay.deleteModule(name);
			release();
			SharedMemoryUtils.closeSharedMemoryFileDescriptor(token);
			name = null;
			token = -1;
			index = -1;
		} else {
			Log.w(TAG, "Not configured; nothing to release.");
		}
	}
	
	/**
	 * @return The name of the module if configured, null if it isn't.
	 */
	public String getName() {
		return name;
	}
	
	protected int getToken() {
		return token;
	}
	
	protected int getIndex() {
		return index;
	}
	
	public abstract boolean hasTimedOut();
	
	protected abstract int getInputChannels();
	protected abstract int getOutputChannels();
	protected abstract boolean configure(String name, int token, int index);
	protected abstract void release();
}
