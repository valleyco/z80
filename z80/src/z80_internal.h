#ifndef Z80_INTERNAL_H
#define Z80_INTERNAL_H

#include "z80.h"
#include "generated/z80_decode.h"

/* Flag bits in F (low byte of AF) */
#define Z80_CF 0x01
#define Z80_NF 0x02
#define Z80_PF 0x04
#define Z80_XF 0x08
#define Z80_HF 0x10
#define Z80_YF 0x20
#define Z80_ZF 0x40
#define Z80_SF 0x80

void z80_insn_done(z80_t *cpu);
void z80_bind_from_decode(z80_t *cpu, const z80_decode_ent *ent, uint8_t opcode);

uint8_t z80_get_r8(z80_t *cpu, uint8_t r);
void z80_set_r8(z80_t *cpu, uint8_t r, uint8_t v);
uint16_t z80_get_rp(z80_t *cpu, uint8_t rp);          /* BC DE HL SP */
void z80_set_rp(z80_t *cpu, uint8_t rp, uint16_t v);
uint16_t z80_get_rp_af(z80_t *cpu, uint8_t rp);       /* BC DE HL AF */
void z80_set_rp_af(z80_t *cpu, uint8_t rp, uint16_t v);
uint16_t z80_hl_or_ixiy(const z80_t *cpu);
void z80_set_hl_or_ixiy(z80_t *cpu, uint16_t v);
uint16_t z80_ea_hl_idx(z80_t *cpu); /* HL or IX/IY+d */

uint8_t z80_get_f(const z80_t *cpu);
void z80_set_f(z80_t *cpu, uint8_t f);
uint8_t z80_get_a(const z80_t *cpu);
void z80_set_a(z80_t *cpu, uint8_t a);

bool z80_eval_cc(const z80_t *cpu, uint8_t cc); /* 3-bit condition */
bool z80_eval_cc_jr(const z80_t *cpu, uint8_t cc); /* 2-bit JR condition */

uint8_t z80_parity(uint8_t v);
void z80_set_sz53p(z80_t *cpu, uint8_t v); /* set S,Z,X,Y,P; clear N; keep C/H caller */

uint8_t z80_mem_rd(z80_t *cpu, uint16_t addr);
void z80_mem_wr(z80_t *cpu, uint16_t addr, uint8_t v);
uint8_t z80_io_rd(z80_t *cpu, uint16_t port);
void z80_io_wr(z80_t *cpu, uint16_t port, uint8_t v);

void z80_alu8(z80_t *cpu, uint8_t op, uint8_t rhs); /* updates A and flags */

#ifdef Z80_TRAP_UNDOC
#include <assert.h>
#define Z80_UNDOC_HIT(cpu) assert(0 && "undocumented opcode")
#else
#define Z80_UNDOC_HIT(cpu) ((void)(cpu))
#endif

#endif /* Z80_INTERNAL_H */
