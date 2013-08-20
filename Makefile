.PHONY: all

all:
	$(MAKE) -C PatchField
	$(MAKE) -C PatchFieldPd
	$(MAKE) -C LowpassSample
	$(MAKE) -C PcmSample
