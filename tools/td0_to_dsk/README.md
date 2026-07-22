# td0_to_dsk

Converts TeleDisk `.td0` images to Kaypro 4/84 DSDD raw `.dsk` format (368640 bytes).

`td0Read.c` and `td0.h` are from [ogdenpm/disktools](https://github.com/ogdenpm/disktools) (GPL-2+).

Build:

```bash
make -C tools/td0_to_dsk
```

Usage:

```bash
tools/td0_to_dsk/td0_to_dsk kaypro/assets/kaypro1.td0 kaypro/assets/kaypro1.dsk
```
