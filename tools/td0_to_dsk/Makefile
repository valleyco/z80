CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11

td0_to_dsk: td0_to_dsk.c td0Read.c utility.c
	$(CC) $(CFLAGS) -o $@ td0_to_dsk.c td0Read.c utility.c

clean:
	rm -f td0_to_dsk

.PHONY: clean
