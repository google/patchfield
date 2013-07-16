all:
	javah -classpath bin/classes -o jni/internal/patchbay.h \
		com.noisepages.nettoyeur.patchbay.Patchbay
	javah -classpath bin/classes -o jni/internal/shared_memory_utils.h \
		com.noisepages.nettoyeur.patchbay.internal.SharedMemoryUtils
	javah -classpath bin/classes -o jni/modules/lowpass.h \
		com.noisepages.nettoyeur.patchbay.modules.LowpassModule
	javah -classpath bin/classes -o jni/modules/identity.h \
		com.noisepages.nettoyeur.patchbay.modules.IdentityModule
	javah -classpath bin/classes -o jni/modules/javamodule.h \
		com.noisepages.nettoyeur.patchbay.modules.JavaModule
	ndk-build
