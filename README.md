# Z80 / Kaypro emulator monorepo

- **`z80/`** — M-cycle Z80 CPU library (`libz80.a`)
- **`emu/`** — generic port-device framework
- **`kaypro/`** — Kaypro 4/84 CP/M machine emulator

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
  --rom kaypro/assets/81-478a.rom \
  --disk-a kaypro/assets/kaypro1.dsk
```

See [kaypro/README.md](kaypro/README.md) for more run examples, and [z80/README.md](z80/README.md) for CPU core details.
