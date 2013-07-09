all:
	javah -classpath bin/classes -o jni/patchbay.h \
		com.noisepages.nettoyeur.patchbay.Patchbay
	javah -classpath bin/classes -o jni/shared_memory_utils.h \
		com.noisepages.nettoyeur.patchbay.SharedMemoryUtils
	javah -classpath bin/classes -o jni/samples/lowpass.h \
		com.noisepages.nettoyeur.patchbay.samples.LowpassModule
	javah -classpath bin/classes -o jni/samples/identity.h \
		com.noisepages.nettoyeur.patchbay.samples.IdentityModule
	ndk-build
