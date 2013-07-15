.PHONY: all

all:
	$(MAKE) -C Patchbay
	$(MAKE) -C PatchbayPd
	$(MAKE) -C PatchbaySource
