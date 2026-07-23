# td0_to_dsk

Converts TeleDisk `.td0` images to Kaypro 4/84 DSDD raw `.dsk` format
(409600 bytes: 40 cyl × 2 sides × 10 sectors × 512).

Sector IDs match the Universal ROM layout: side 0 uses 0–9, side 1 uses 10–19.
Sector 0 is the cold-boot record (checksummed by `BOOTSYS`).

`td0Read.c` and `td0.h` are from [ogdenpm/disktools](https://github.com/ogdenpm/disktools) (GPL-2+).

Build:

```bash
make -C tools/td0_to_dsk
```

Usage:

```bash
tools/td0_to_dsk/td0_to_dsk kaypro/assets/kaypro1.td0 kaypro/assets/kaypro1.dsk
```
