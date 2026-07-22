CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O0 -g -I.

SRC = port_bus.c
OBJ = $(SRC:.c=.o)
LIB = libemu.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJ)
	ar rcs $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(LIB)
