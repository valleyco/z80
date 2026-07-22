# ASM-Driven Z80 Test Harness

Assembly-authored integration tests for the M-cycle Z80 core. Tests are written in **asmx** Z80 assembly, assembled to `.bin`/`.lst`, converted to a binary vector file by Python, and executed by a generic C runner.

## Workflow

```text
tests/test.asm  (+ @test / @init / @expect comments)
    → asmx -C Z80 -l -b
    → tests/test.bin + tests/test.lst
    → tools/gen_test_vectors.py
    → tests/test_vectors.bin
    → tests/test_runner
```

```bash
make test              # assemble, generate vectors, run runner
make dump-vectors      # human-readable vector dump
make test-legacy       # original hand-written test_pilot.c
```

## Comment DSL

Parse directives from `; @...` comments in the `.asm` source. Addresses are cross-checked against the `.lst` listing.

| Directive | Purpose |
|-----------|---------|
| `@test name` | Start a new test; the label matching `name` (case-insensitive) is `start_pc` |
| `@init key=val ...` | CPU state after `z80_reset`; only listed fields are applied |
| `@mem addr=val` | Preload RAM (repeatable) |
| `@io port=val` | Preload I/O port (repeatable) |
| `@expect key=val ...` | Post-run checks; only listed fields are verified |
| `@expect_mem addr=val` | Post-run memory check |
| `@end` | Optional explicit end address marker |

### Value syntax

- Hex: `0100h`, `0x0100`, `$0100`
- 16-bit registers: `af`, `bc`, `de`, `hl`, `ix`, `iy`, `sp`, `pc`, `af_`, `bc_`, `de_`, `hl_`
- 8-bit registers: `a`, `f`, `b`, `c`, `d`, `e`, `h`, `l`
- Special: `i`, `r`, `im`, `iff1`, `iff2`
- Flags: `flag.s`, `flag.z`, `flag.y`, `flag.x`, `flag.h`, `flag.pv`, `flag.n`, `flag.c`
- Meta: `halted=1`, `trap=1` (expect instruction-fetch guard trap)

Half-registers (`b`/`c`/`d`/`e`/`h`/`l`) may be mixed with pair inits; half-reg values are applied after pair assigns. Example:

```asm
; @init bc=1234h b=56h
; @expect b=56h c=34h
```

### Example

```asm
        .cpu Z80

; @test ld_a_ix_disp
; @init pc=0100h ix=3000h
; @mem 3005h=99h
; @expect a=99h
        org 0100h
ld_a_ix_disp:
        dd
        ld      a,(ix+5)
        halt
```

Each test **must** end with `halt`. The runner stops when `cpu.halted` is true.

### Code boundaries

- `start_pc` = label address from listing
- `end_pc` = byte after the `halt` instruction (exclusive)
- **Fetch guard:** an instruction fetch (M1 / M1_CB / M1_ED) outside `[start_pc, end_pc)` sets `fetch_trap` and stops the test (failure unless `@expect trap=1`)

Data reads/writes and stack access outside the code region are allowed.

## Binary vector format (`test_vectors.bin`)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Magic `Z80T` |
| 4 | 1 | Version (1) |
| 5 | 2 | Test count (LE u16) |
| 7 | 4 | Total code blob size (LE u32) |

Per test record:

| Field | Size |
|-------|------|
| name_len | u16 |
| name | UTF-8 |
| start_pc, end_pc | u16 each |
| code_off, code_len | u32 each (20 bytes for `HHIIII` header) |
| init_mask, expect_mask | u32 each |
| init_state | 36 bytes |
| expect_state | 36 bytes |
| mem_count, io_count | u16 each |
| mem pairs | mem_count × (u16 addr, u8 val) |
| io pairs | io_count × (u16 port, u8 val) |
| expect_mem_count | u16 |
| expect_mem pairs | expect_mem_count × (u16 addr, u8 val) |

File tail: concatenated code bytes (`total_code_bytes`).

### CPU state blob (36 bytes)

| Offset | Field |
|--------|-------|
| 0–7 | af, bc, de, hl |
| 8–15 | ix, iy, sp, pc |
| 16–23 | af_, bc_, de_, hl_ |
| 24–28 | i, r, im, iff1, iff2 |
| 29 | flags (individual flag expect bits) |
| 30 | r8_mask (half-register select: bit0=b … bit5=l) |
| 31–35 | padding |

### Mask bits

| Bit | Field |
|-----|-------|
| 0–15 | af, bc, de, hl, ix, iy, sp, pc, af_, bc_, de_, hl_, i, r, im, iff1 |
| 16 | iff2 |
| 17 | halted |
| 18 | fetch_trap |
| 19 | a (8-bit, high byte of af) |
| 20 | f (8-bit, low byte of af) |
| 24–31 | flag.s, flag.z, flag.y, flag.x, flag.h, flag.pv, flag.n, flag.c |

Half-registers `b`/`c`/`d`/`e`/`h`/`l` are selected by `r8_mask` in the state blob (not main mask bits). Values live in the high/low bytes of `bc`/`de`/`hl`.

## Tools

- **`tools/gen_test_vectors.py`** — parse `.asm` + `.lst` + `.bin` → `test_vectors.bin`
- **`tools/dump_test_vectors.py`** — print vectors in readable form (hex code, no disassembly)

## Runner behavior

1. `z80_reset`
2. Apply init fields from vector
3. Load test code into bus memory at `start_pc`
4. Apply `@mem` / `@io` preloads
5. Set fetch guard `[start_pc, end_pc)`
6. Run `z80_step_m` until `halted`, `fetch_trap`, or 100000 steps
7. Compare `@expect` / `@expect_mem` fields
8. Report `FAIL name: reason` on mismatch

## CI note

Commit `tests/test_vectors.bin` so CI can run `test_runner` without asmx. Regenerate locally after editing `test.asm`.
