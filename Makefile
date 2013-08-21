.PHONY: all

all:
	$(MAKE) -C Patchfield
	$(MAKE) -C PatchfieldPd
	$(MAKE) -C LowpassSample
	$(MAKE) -C PcmSample
