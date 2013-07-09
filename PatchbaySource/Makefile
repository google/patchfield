all:
	javah -classpath bin/classes -o jni/pcmsource.h com.noisepages.nettoyeur.patchbay.source.PcmSource
	env NDK_MODULE_PATH=$(CURDIR)/.. ndk-build
