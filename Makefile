.PHONY: all

all:
	$(MAKE) -C Patchfield
	$(MAKE) -C PatchfieldPd
	$(MAKE) -C LowpassSample
	$(MAKE) -C MessageSample
	$(MAKE) -C PcmSample
