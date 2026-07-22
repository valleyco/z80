CC = gcc
AR = ar
CFLAGS = -std=c11 -Wall -Wextra -Werror -O0 -g \
	-Iinclude -Isrc -Isrc/generated -Itests

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
LIB = libz80.a

ASM = tests/test.asm
ASM_BIN = tests/test.bin
ASM_LST = tests/test.lst
VECTORS = tests/test_vectors.bin

.PHONY: all clean test test-legacy gen dump-vectors asm vectors lib

all: lib tests/test_runner

lib: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $(OBJ)

gen:
	python3 tools/gen_z80_core.py

asm: $(ASM_BIN) $(ASM_LST)

$(ASM_BIN) $(ASM_LST): $(ASM)
	asmx -e -w -C Z80 -l $(ASM_LST) -b -o $(ASM_BIN) $(ASM)

vectors: $(VECTORS)

$(VECTORS): $(ASM) $(ASM_BIN) $(ASM_LST) tools/gen_test_vectors.py
	python3 tools/gen_test_vectors.py $(ASM) -o $(VECTORS)

tests/test_runner: tests/test_runner.c tests/test_vectors.c tests/test_vectors.h $(LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_runner.c tests/test_vectors.c -L. -lz80

tests/test_pilot: tests/test_pilot.c $(LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_pilot.c -L. -lz80

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: tests/test_runner $(VECTORS)
	./tests/test_runner $(VECTORS)

test-legacy: tests/test_pilot
	./tests/test_pilot

dump-vectors: $(VECTORS)
	python3 tools/dump_test_vectors.py $(VECTORS)

clean:
	rm -f $(OBJ) $(LIB) tests/test_pilot tests/test_runner tests/test_vectors.o
	rm -f $(ASM_BIN) $(ASM_LST) $(VECTORS)
