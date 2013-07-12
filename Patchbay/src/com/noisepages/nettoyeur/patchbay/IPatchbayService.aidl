package com.noisepages.nettoyeur.patchbay;

import com.noisepages.nettoyeur.patchbay.IPatchbayClient;

import java.util.List;

/**
 * Patchbay service interface, implemented by {@link Patchbay}.
 */
interface IPatchbayService {
	
	/**
	 * Registers a Patchbay client. Note that a client is not an audio module.
	 * Rather, clients are Java objects that will be notified when the state
	 * of the patchbay changes.
	 */
	void registerClient(IPatchbayClient client);
	    
	/**
	 * Unregisters a Patchbay client.
	 */
	void unregisterClient(IPatchbayClient client);
	
	/**
	 * Starts audio rendering.
	 *
	 * @return 0 on success, or a negative error code on failure.
	 */
	int start();
	    
	/**
	 * Stops audio rendering.
	 */
	void stop();
	    
	/**
	 * Activates the given audio module.
	 *
	 * @return 0 on success, or a negative error code on failure.
	 */
	int activateModule(String module);
	    
	/**
	 * Deactivates the given audio module.
	 *
	 * @return 0 on success, or a negative error code on failure.
	 */
	int deactivateModule(String module);
	    
	/**
	 * Connects the given source port to the given sink port.
	 *
	 * @return 0 on success, or a negative error code on failure.
	 */
	int connectModules(String source, int sourcePort, String sink, int sinkPort);
	    
	/**
	 * Disconnects the given source port from the given sink port.
	 *
	 * @return 0 on success, or a negative error code on failure.
	 */
	int disconnectModules(String source, int sourcePort, String sink, int sinkPort);
	    
	/**
	 * @return True if the Patchbay is currently rendering audio.
	 */
	boolean isRunning();
	    
	/**
	 * @return The list of currently registered audio modules. 
	 */
	List<String> getModules();
	    
	/**
	 * @return The number of input channels of the given module, or a negative error
	 * code if the module doesn't exist.
	 */
	int getInputChannels(String module);
	    
	/**
	 * @return The number of output channels of the given module, or a negative error
	 * code if the module doesn't exist.
	 */
	int getOutputChannels(String module);
	    
	/**
	 * @return True if the given module is currently active.
	 */
	boolean isActive(String module);
	    
	/**
	 * @return True if the source port is directly connected to the sink port.
	 */
	boolean isConnected(String source, int sourcePort, String sink, int sinkPort);
	    
	/**
	 * @return True if the input of of the given sink depends on the output of the given source,
	 * directly or indirectly. In other words, this method returns true if the source must have
	 * produced its output before the sink can run.
	 */
	boolean isDependent(String sink, String source);
	
	/**
	 * @return The sample rate in Hz at which the Patchbay operates. This value is determined
	 * by the hardware and cannot be changed. Audio modules should be able to operate at 44.1kHz
	 * as well as 48kHz.
	 */
	int getSampleRate();
	    
	/**
	 * @return The buffer size in frames at which the Patchbay operates. This value is determined
	 * by the hardware and cannot be changed. Audio modules should make no assumptions about the
	 * buffer size. In particular, it will not be a power of two on many modules.
	 */
	int getBufferSize();
	    
	/**
	 * Creates a new audio module in the Patchbay service; for internal use mostly, to be called by
	 * the configure method of {@link AudioModule}.
	 *
	 * @return The index of the new module on success, or a negative error code on failure.
	 */
	int createModule(String module, int inputChannels, int outputChannels);
	    
	/**
	 * Deletes an audio module from the Patchbay service; for internal use mostly, to be called by
	 * the release method of {@link AudioModule}.
	 *
	 * @return 0 on success, or a negative error code on failure.
	 */
	int deleteModule(String module);
	    
	/**
	 * @return The native protocol version; for internal use only.
	 */
	int getProtocolVersion();
	
	/**
	 * Passes the ashmem file descriptor through a Unix domain socket; for internal use only.
	 *
	 * @return 0 on success, or a negative error code on failure.
	 */
	int sendSharedMemoryFileDescriptor();
}
