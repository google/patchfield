all:
	javah -classpath bin/classes -o jni/module/pdmodule.h com.noisepages.nettoyeur.patchbay.pd.PdModule
	env NDK_MODULE_PATH=$(CURDIR)/.. ndk-build
