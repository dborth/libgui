.PHONY = all wii gc wiiu wii-clean gc-clean wiiu-clean

all: wii gc wiiu

clean: wii-clean gc-clean wiiu-clean

wii:
	$(MAKE) -f Makefile.wii

wii-clean:
	$(MAKE) -f Makefile.wii clean

gc:
	$(MAKE) -f Makefile.gc

gc-clean:
	$(MAKE) -f Makefile.gc clean

wiiu:
	$(MAKE) -f Makefile.wiiu

wiiu-clean:
	$(MAKE) -f Makefile.wiiu clean