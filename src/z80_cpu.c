#include <string.h>

#include "generated/z80_decode.h"
#include "generated/z80_dispatch.h"
#include "z80_internal.h"

void z80_init(z80_t *cpu, z80_bus_t bus) {
  memset(cpu, 0, sizeof(*cpu));
  cpu->bus = bus;
  z80_reset(cpu);
}

void z80_reset(z80_t *cpu) {
  cpu->af = cpu->bc = cpu->de = cpu->hl = 0xFFFF;
  cpu->af_ = cpu->bc_ = cpu->de_ = cpu->hl_ = 0xFFFF;
  cpu->ix = cpu->iy = 0xFFFF;
  cpu->sp = 0xFFFF;
  cpu->pc = 0;
  cpu->i = cpu->r = 0;
  cpu->iff1 = cpu->iff2 = 0;
  cpu->im = 0;
  cpu->halted = false;
  cpu->phase = Z80_PH_M1;
  cpu->index = Z80_IDX_NONE;
  cpu->disp = 0;
  cpu->cb_after_index = false;
  cpu->family = FAM_NOP;
  cpu->m_index = 0;
  cpu->m_count = 0;
  cpu->block_repeat = false;
}

static void bump_r(z80_t *cpu) {
  /* V1: approximate refresh — increment low 7 bits */
  uint8_t r7 = (uint8_t)(cpu->r & 0x80);
  cpu->r = (uint8_t)(r7 | (((cpu->r + 1) & 0x7F)));
}

static bool op_needs_disp(const z80_t *cpu, const z80_decode_ent *ent) {
  if (cpu->index == Z80_IDX_NONE) return false;
  switch (ent->family) {
  case FAM_LD_R_R:
  case FAM_ALU_A_R:
  case FAM_INC_R:
  case FAM_DEC_R:
  case FAM_LD_R_N:
    /* needs d if (HL) involved — checked after bind for LD_R_R; for fetch path
       before bind, use opcode bits */
    break;
  default:
    break;
  }
  /* Heuristic from opcode before full bind: (HL) encoding bit pattern */
  (void)ent;
  return false;
}

static bool opcode_uses_hl_mem(uint8_t opcode, z80_family_t fam) {
  switch (fam) {
  case FAM_LD_R_R: {
    uint8_t d = (uint8_t)((opcode >> 3) & 7);
    uint8_t s = (uint8_t)(opcode & 7);
    return d == 6 || s == 6;
  }
  case FAM_ALU_A_R:
    return (opcode & 7) == 6;
  case FAM_INC_R:
  case FAM_DEC_R:
  case FAM_LD_R_N:
    return ((opcode >> 3) & 7) == 6;
  case FAM_EX_ISP_HL:
  case FAM_JP_HL:
  case FAM_LD_SP_HL:
  case FAM_ADD_HL_RP:
  case FAM_LD_INN_HL:
  case FAM_LD_HL_INN:
    return true; /* remaps HL register / (SP) exchange — no displacement */
  default:
    return false;
  }
}

static bool family_needs_disp(z80_family_t fam, uint8_t opcode) {
  switch (fam) {
  case FAM_LD_R_R:
  case FAM_ALU_A_R:
  case FAM_INC_R:
  case FAM_DEC_R:
  case FAM_LD_R_N:
    return opcode_uses_hl_mem(opcode, fam);
  default:
    return false;
  }
}

static void enter_exec(z80_t *cpu, const z80_decode_ent *ent) {
  /* m_count already set in bind; finalize now that disp/prefix done */
  if (cpu->m_count <= 1) {
    cpu->m_index = 1;
    z80_dispatch_exec(cpu);
  } else {
    cpu->m_index = 2;
    cpu->phase = Z80_PH_EXEC;
  }
  (void)ent;
}

static void decode_and_enter(z80_t *cpu, const z80_decode_ent *ent, uint8_t opcode) {
  z80_bind_from_decode(cpu, ent, opcode);

  if (cpu->family == FAM_UNDOC || cpu->family == FAM_CB_UNDOC ||
      cpu->family == FAM_ED_NOP) {
    Z80_UNDOC_HIT(cpu);
  }

  if (cpu->family == FAM_HALT) {
    cpu->halted = true;
    z80_insn_done(cpu);
    return;
  }

  enter_exec(cpu, ent);
}

static void phase_m1(z80_t *cpu) {
  uint8_t opcode = z80_mem_rd(cpu, cpu->pc++);
  bump_r(cpu);

  if (cpu->index == Z80_IDX_NONE) {
    const z80_decode_ent *ent = &z80_decode_root[opcode];
    switch (ent->family) {
    case FAM_PREFIX_CB:
      cpu->phase = Z80_PH_M1_CB;
      return;
    case FAM_PREFIX_ED:
      cpu->phase = Z80_PH_M1_ED;
      return;
    case FAM_PREFIX_DD:
      cpu->index = Z80_IDX_IX;
      return; /* stay M1 */
    case FAM_PREFIX_FD:
      cpu->index = Z80_IDX_IY;
      return; /* stay M1 */
    default:
      break;
    }

    decode_and_enter(cpu, ent, opcode);
    return;
  }

  /* Indexed context: DD/FD already latched — DD/FD bytes are opcodes, not re-latch */
  if (opcode == 0xCB) {
    cpu->cb_after_index = true;
    cpu->phase = Z80_PH_M_DISP;
    return;
  }
  if (opcode == 0xED) {
    cpu->phase = Z80_PH_M1_ED; /* index preserved */
    return;
  }
  if (opcode == 0xDD || opcode == 0xFD) {
    /* Treat as undocumented/NOP-like in indexed context for V1 */
    const z80_decode_ent *ent = &z80_decode_root[opcode];
    z80_bind_from_decode(cpu, ent, opcode);
    cpu->family = FAM_UNDOC;
    Z80_UNDOC_HIT(cpu);
    z80_insn_done(cpu);
    return;
  }

  {
    const z80_decode_ent *ent = &z80_decode_root[opcode];
    if (family_needs_disp((z80_family_t)ent->family, opcode)) {
      cpu->opcode = opcode;
      cpu->family = (z80_family_t)ent->family;
      cpu->tmp8 = opcode; /* stash for after disp */
      cpu->phase = Z80_PH_M_DISP;
      return;
    }
    decode_and_enter(cpu, ent, opcode);
  }
  (void)op_needs_disp;
}

static void phase_m1_cb(z80_t *cpu) {
  uint8_t opcode = z80_mem_rd(cpu, cpu->pc++);
  bump_r(cpu);
  const z80_decode_ent *ent = &z80_decode_cb[opcode];
  z80_bind_from_decode(cpu, ent, opcode);
  /* Indexed CB: (HL) means (IX/IY+d); disp already read */
  if (cpu->index != Z80_IDX_NONE && (opcode & 7) == 6) {
    cpu->src.kind = Z80_OP_MEM_HL;
    cpu->dst.kind = Z80_OP_MEM_HL;
    cpu->m_count = (uint8_t)(cpu->m_count < 2 ? 2 : cpu->m_count);
  }
  decode_and_enter(cpu, ent, opcode);
}

static void phase_m1_ed(z80_t *cpu) {
  uint8_t opcode = z80_mem_rd(cpu, cpu->pc++);
  bump_r(cpu);
  const z80_decode_ent *ent = &z80_decode_ed[opcode];
  decode_and_enter(cpu, ent, opcode);
}

static void phase_m_disp(z80_t *cpu) {
  cpu->disp = (int8_t)z80_mem_rd(cpu, cpu->pc++);
  if (cpu->cb_after_index) {
    cpu->phase = Z80_PH_M1_CB;
    return;
  }
  /* Indexed non-CB: opcode was stashed in tmp8 before M_DISP */
  {
    uint8_t opcode = cpu->tmp8;
    const z80_decode_ent *ent = &z80_decode_root[opcode];
    z80_bind_from_decode(cpu, ent, opcode);
    decode_and_enter(cpu, ent, opcode);
  }
}

static void phase_exec(z80_t *cpu) {
  z80_dispatch_exec(cpu);
}

void z80_step_m(z80_t *cpu) {
  if (cpu->halted) return;

  switch (cpu->phase) {
  case Z80_PH_M1:
    phase_m1(cpu);
    break;
  case Z80_PH_M1_CB:
    phase_m1_cb(cpu);
    break;
  case Z80_PH_M1_ED:
    phase_m1_ed(cpu);
    break;
  case Z80_PH_M_DISP:
    phase_m_disp(cpu);
    break;
  case Z80_PH_EXEC:
    phase_exec(cpu);
    break;
  }
}

void z80_run_m(z80_t *cpu, unsigned n) {
  while (n-- > 0 && !cpu->halted) z80_step_m(cpu);
}
