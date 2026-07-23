# Kaypro 4/84 emulator

Terminal-first CP/M machine built on `libz80` and the shared `emu/` port-device framework.

## Layers

| Layer | Path | Rule |
|-------|------|------|
| CPU | `../z80/` | Freestanding; bus callbacks only |
| Port framework | `../emu/` | Freestanding port mux |
| Machine + devices | `src/machine/`, `src/devices/` | Pure Kaypro; I/O via host ops / memory blobs |
| Host (POSIX) | `src/host/posix/` | CLI, `fopen`, stdin/stdout; later `host/esp32/` |

Host console/keyboard/logging is injected with `kaypro_set_host()` / `kaypro_host_posix_install()`.
ROM and disks load as bytes (`kaypro_load_rom_bytes`, `kaypro_attach_disk_mem`); path helpers live in the POSIX host.

## Build

```bash
make -C ..          # from repo root
make -C .. kaypro   # this target only
make test           # smoke tests
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
| `src/devices/` | sysport, SIO, FDC1793, keyboard, CRT (SY6545), HDC stub |
| `src/host/kaypro_host.h` | Host ops + path-loader declarations |
| `src/host/posix/` | Desktop CLI, stdin/stdout, file loaders |
| `tests/smoke_test.c` | Bank-1 ROM read regression |
| `tests/crt_smoke_test.c` | SY6545 port ready/regs/char echo |
| `tests/hdc_smoke_test.c` | WD1002 absent-controller fail-fast |

## Status

Phase 1: memory banking, port dispatch, FDC/SIO, NMI on HALT, CRT echo
(Universal 1Ch-1Fh), and a WD1002-HD0 stub at 80h-87h that fails `winrest`
so ROM falls back to floppies.

Universal ROM boots CP/M from a 40×2×10×512 `.dsk` and reaches the `A0>`
prompt. Console echo follows CRT data-port writes (cursor moves are not
translated to host newlines).
