all:
	-javah -classpath bin/classes -o jni/module/pdmodule.h com.noisepages.nettoyeur.patchfield.pd.PdModule
	env NDK_MODULE_PATH=$(CURDIR)/.. ndk-build
