.PHONY: all

all:
	$(MAKE) -C PatchField
	$(MAKE) -C LowpassSample
	$(MAKE) -C PatchbayPd
	$(MAKE) -C PatchbayPcmSample
