# td0_to_dsk

Converts TeleDisk `.td0` images to Kaypro raw `.dsk` sector images.

## Formats

| `--format` | Geometry | Size | Notes |
|------------|----------|------|-------|
| `ssdd` | 40 × 1 × 10 × 512 | 204800 | Linear SS; sector IDs 0–9 on head 0. Matches cpmtools `kpii`. Track N at offset `N*10*512`. |
| `dsdd` | 40 × 2 × 10 × 512 | 409600 | Universal DSDD. Side 0 IDs 0–9, side 1 IDs 10–19. `offset = ((cyl*2+head)*10+phys)*512`. |
| `auto` (default) | from TD0 `surfaces` | | `surfaces < 2` → `ssdd`, else `dsdd`. |

Sector 0 on side 0 is the cold-boot record (checksummed by `BOOTSYS`).

Native single-sided images stay SSDD. To use SS software under Universal/Kaypro 4
CP/M (2K allocation blocks), rewrite with `tools/ssdd_to_dsdd` (cpmtools
`kpii` → `kpiv`).

`td0Read.c` and `td0.h` are from [ogdenpm/disktools](https://github.com/ogdenpm/disktools) (GPL-2+).

## Build

```bash
make -C tools/td0_to_dsk
```

## Usage

```bash
tools/td0_to_dsk/td0_to_dsk --format auto kaypro/assets/td0/kaypro1.td0 /tmp/kaypro1.dsk
tools/td0_to_dsk/td0_to_dsk --format ssdd kii-mbas.td0 kaypro/assets/images/ssdd/kii-mbas.dsk
tools/td0_to_dsk/td0_to_dsk --format dsdd kaypro1.td0 kaypro/assets/images/dsdd/kaypro1.dsk
```
