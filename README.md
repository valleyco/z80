# Z80 / Kaypro emulator monorepo

- **`z80/`** — M-cycle Z80 CPU library (`libz80.a`), freestanding
- **`emu/`** — generic port-device framework, freestanding
- **`kaypro/`** — Kaypro 4/84 CP/M machine + host backends
  - portable: `kaypro/src/machine`, `kaypro/src/devices`
  - desktop host: `kaypro/src/host/posix` (ESP32 host can plug in later via the same `kaypro_host_ops_t`)

## Build

```bash
make            # libz80 + kaypro_run
make test       # CPU instruction tests (56 asm vectors)
make kaypro     # kaypro executable only
```

## Run Kaypro (requires ROM + disk images)

Paths are relative to your current working directory. Fetch assets first, then run from the repo root:

```bash
bash tools/fetch_kaypro_assets.sh
./kaypro/kaypro_run \
  --rom kaypro/assets/rom/81-478a.rom \
  --disk-a kaypro/assets/images/dsdd/kaypro1.dsk
```

See [kaypro/README.md](kaypro/README.md) for layout, host ops, and more run examples, and [z80/README.md](z80/README.md) for CPU core details.
