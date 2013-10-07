all:
	-javah -classpath bin/classes -o jni/message.h \
	  com.noisepages.nettoyeur.patchfield.messagesample.MessageModule
	env NDK_MODULE_PATH=$(CURDIR)/.. ndk-build
