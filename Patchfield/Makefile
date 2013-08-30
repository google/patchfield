all:
	-javah -classpath bin/classes -o jni/internal/audio_module_java.h \
		com.noisepages.nettoyeur.patchfield.AudioModule
	-javah -classpath bin/classes -o jni/internal/patchfield.h \
		com.noisepages.nettoyeur.patchfield.Patchfield
	-javah -classpath bin/classes -o jni/internal/shared_memory_utils.h \
		com.noisepages.nettoyeur.patchfield.internal.SharedMemoryUtils
	ndk-build
