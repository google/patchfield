all:
	javah -classpath bin/classes -o jni/lowpass.h \
		com.noisepages.nettoyeur.patchbay.lowpass.LowpassModule
	env NDK_MODULE_PATH=$(CURDIR)/.. ndk-build
