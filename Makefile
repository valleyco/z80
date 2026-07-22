.PHONY: all z80 kaypro test clean

all: z80 kaypro

z80:
	$(MAKE) -C z80

kaypro: z80 emu
	$(MAKE) -C kaypro

emu:
	$(MAKE) -C emu

test:
	$(MAKE) -C z80 test

clean:
	$(MAKE) -C z80 clean
	$(MAKE) -C emu clean
	$(MAKE) -C kaypro clean
