CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O0 -g -Isrc -Isrc/generated
SRC = \
	src/z80_cpu.c \
	src/z80_regs.c \
	src/z80_base.c \
	src/z80_cb.c \
	src/z80_ed.c \
	src/generated/z80_decode_root.c \
	src/generated/z80_decode_cb.c \
	src/generated/z80_decode_ed.c \
	src/generated/z80_dispatch.c

OBJ = $(SRC:.c=.o)

.PHONY: all clean test gen

all: tests/test_pilot

gen:
	python3 tools/gen_z80_core.py

tests/test_pilot: tests/test_pilot.c $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: tests/test_pilot
	./tests/test_pilot

clean:
	rm -f $(OBJ) tests/test_pilot
