#ifndef Z80_H
#define Z80_H

#include <stdbool.h>
#include <stdint.h>

#include "generated/z80_families.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  Z80_PH_M1,
  Z80_PH_M1_CB,
  Z80_PH_M1_ED,
  Z80_PH_M_DISP, /* read d after DD/FD before op or before M1_CB */
  Z80_PH_EXEC    /* M2..Mn: dispatch by family_id + m_index */
} z80_phase_t;

typedef enum { Z80_IDX_NONE, Z80_IDX_IX, Z80_IDX_IY } z80_index_t;

typedef enum {
  Z80_OP_NONE = 0,
  Z80_OP_REG8,
  Z80_OP_REG16,
  Z80_OP_REG16_AF, /* push/pop AF encoding */
  Z80_OP_MEM_HL,   /* (HL) or remapped (IX/IY+d) */
  Z80_OP_MEM_BC,
  Z80_OP_MEM_DE,
  Z80_OP_IMM8,
  Z80_OP_IMM16,
  Z80_OP_ADDR16,
  Z80_OP_PORT8
} z80_opnd_kind_t;

typedef struct {
  z80_opnd_kind_t kind;
  uint8_t reg; /* reg8 0..7 or rp 0..3 */
} z80_opnd_t;

typedef struct {
  uint8_t (*mem_read)(void *ctx, uint16_t addr);
  void (*mem_write)(void *ctx, uint16_t addr, uint8_t v);
  uint8_t (*io_read)(void *ctx, uint16_t port);
  void (*io_write)(void *ctx, uint16_t port, uint8_t v);
  void *bus_ctx;
} z80_bus_t;

typedef struct z80 {
  /* architectural */
  uint16_t af, bc, de, hl, ix, iy, sp, pc;
  uint16_t af_, bc_, de_, hl_;
  uint8_t i, r;
  uint8_t iff1, iff2, im;
  bool halted;

  /* bus */
  z80_bus_t bus;

  /* M-machine */
  z80_phase_t phase;
  z80_index_t index;   /* DD/FD latch; cleared when instruction completes */
  int8_t disp;         /* (IX/IY+d) */
  bool cb_after_index; /* DD CB d op / FD CB d op */

  z80_family_t family;
  uint8_t m_index; /* 2 = first post-decode M, etc.; 1 used when finishing in M1 */
  uint8_t m_count; /* planned Ms for this activation (may change for cond/block) */

  /* decoded work context — filled in M1*, consumed blindly in EXEC */
  z80_opnd_t dst, src;
  uint8_t alu_op;
  uint8_t cc;
  bool cond_true;
  uint8_t tmp8;
  uint16_t tmp16;
  uint16_t addr;
  bool block_repeat; /* ED block ops */
  uint8_t bit;       /* CB bit index */
  uint8_t opcode;    /* last opcode byte (for field re-extract if needed) */
} z80_t;

void z80_init(z80_t *cpu, z80_bus_t bus);
void z80_reset(z80_t *cpu);
void z80_step_m(z80_t *cpu);
void z80_run_m(z80_t *cpu, unsigned n);

#ifdef __cplusplus
}
#endif

#endif /* Z80_H */
