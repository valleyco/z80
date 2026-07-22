# Kaypro 4/84 emulator

Terminal-first CP/M machine built on `libz80` and the shared `emu/` port-device framework.

## Build

```bash
make -C ..          # from repo root
make -C .. kaypro   # this target only
make test           # smoke test (memory banking)
```

## Prerequisites

Download the Universal ROM and CP/M disk (from repo root):

```bash
bash tools/fetch_kaypro_assets.sh
```

That places `81-478a.rom` and `kaypro1.dsk` under `assets/`.

## Run

Paths are relative to your current working directory:

```bash
# from repo root
./kaypro/kaypro_run --rom kaypro/assets/81-478a.rom --disk-a kaypro/assets/kaypro1.dsk

# from kaypro/ directory
./kaypro_run --rom assets/81-478a.rom --disk-a assets/kaypro1.dsk

# log IN/OUT during bring-up
./kaypro_run --rom assets/81-478a.rom --disk-a assets/kaypro1.dsk --trace
```

## Layout

| Path | Role |
|------|------|
| `src/machine/` | Kaypro wiring, memory map, port registration |
| `src/devices/` | sysport, SIO, FDC1793, keyboard |
| `src/main.c` | CLI entry point |
| `tests/smoke_test.c` | Bank-1 ROM read regression |

## Status

Phase 1 skeleton: memory banking, port dispatch, simplified FDC/SIO, NMI on HALT.
CRT (6845) deferred to phase 2.
