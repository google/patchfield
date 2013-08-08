.PHONY: all

all:
	$(MAKE) -C Patchbay
	$(MAKE) -C PatchbayLowpassSample
	$(MAKE) -C PatchbayPd
	$(MAKE) -C PatchbayPcmSample
