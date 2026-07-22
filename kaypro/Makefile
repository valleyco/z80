CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O0 -g \
	-Iinclude \
	-I../emu \
	-I../z80/include \
	-I../z80/src \
	-I../z80/src/generated \
	-Isrc/machine \
	-Isrc/devices

LDFLAGS = -L../z80 -L../emu -lz80 -lemu

MACHINE_SRC = \
	src/machine/kaypro.c \
	src/machine/kaypro_mem.c \
	src/machine/kaypro_ports.c

DEVICE_SRC = \
	src/devices/sysport.c \
	src/devices/sio.c \
	src/devices/fdc1793.c \
	src/devices/keyboard.c \
	src/devices/crt6845.c

MAIN_SRC = src/main.c
SMOKE_SRC = tests/smoke_test.c

.PHONY: all clean test smoke

all: ../emu/libemu.a ../z80/libz80.a kaypro_run

../emu/libemu.a:
	$(MAKE) -C ../emu

../z80/libz80.a:
	$(MAKE) -C ../z80 lib

kaypro_run: $(MACHINE_SRC) $(DEVICE_SRC) $(MAIN_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

smoke: ../emu/libemu.a ../z80/libz80.a tests/smoke_test
	./tests/smoke_test

tests/smoke_test: $(MACHINE_SRC) $(DEVICE_SRC) $(SMOKE_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: smoke

clean:
	rm -f kaypro_run tests/smoke_test
