# Z80 M-cycle simulator (V1)

C11 M-cycle-accurate Z80 core for CPM-class machines. See [docs/z80_m_cycle_simulator_design.md](docs/z80_m_cycle_simulator_design.md).

## Build / test

```bash
make lib                        # build libz80.a
make test                       # assemble tests/test.asm, run asm test runner
make test-legacy                # original hand-written test_pilot.c
make dump-vectors               # human-readable dump of tests/test_vectors.bin
```

Requires [asmx](https://sourceforge.net/projects/asmx/) on PATH to regenerate vectors from `tests/test.asm`. The committed `tests/test_vectors.bin` lets CI run without asmx.

## Layout

| Path | Role |
|------|------|
| `include/z80.h` | Public API + CPU state |
| `src/z80_cpu.c` | Phase loop (M1 / CB / ED / DISP / EXEC) |
| `src/z80_regs.c` | Registers, flags, bind helpers |
| `src/z80_base.c` / `z80_cb.c` / `z80_ed.c` | Hand-written M-handlers |
| `src/generated/` | Decode tables, family enum, dispatch (generator only) |
| `tools/gen_z80_core.py` | Generator from `z80_simple*.json` |

## Public API

- `z80_init` / `z80_reset`
- `z80_step_m` — one M-cycle (no-op while halted)
- `z80_run_m` — N M-cycles
- `z80_nmi` / `z80_irq_im1` — interrupt delivery (machine layer)
