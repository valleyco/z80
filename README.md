# Z80 M-cycle simulator (V1)

C11 M-cycle-accurate Z80 core for CPM-class machines. See [docs/z80_m_cycle_simulator_design.md](docs/z80_m_cycle_simulator_design.md).

## Build / test

```bash
python3 tools/gen_z80_core.py   # regenerate src/generated/
make test                       # build + run pilot tests
```

## Layout

| Path | Role |
|------|------|
| `src/z80.h` | Public API + CPU state |
| `src/z80_cpu.c` | Phase loop (M1 / CB / ED / DISP / EXEC) |
| `src/z80_regs.c` | Registers, flags, bind helpers |
| `src/z80_base.c` / `z80_cb.c` / `z80_ed.c` | Hand-written M-handlers |
| `src/generated/` | Decode tables, family enum, dispatch (generator only) |
| `tools/gen_z80_core.py` | Generator from `z80_simple*.json` |

## Public API

- `z80_init` / `z80_reset`
- `z80_step_m` — one M-cycle (no-op while halted)
- `z80_run_m` — N M-cycles
