#include "z80_internal.h"
#include "generated/z80_handlers.h"

static void z80_cb_finish(z80_t *cpu) { z80_insn_done(cpu); }

static void z80_cb_advance(z80_t *cpu) { cpu->m_index++; }

static void z80_cb_set_flags(z80_t *cpu, uint8_t result, uint8_t carry) {
  uint8_t f = z80_get_f(cpu);
  f = (uint8_t)(carry ? (f | Z80_CF) : (f & (uint8_t)~Z80_CF));
  f = (uint8_t)(f & (uint8_t)~(Z80_SF | Z80_ZF | Z80_YF | Z80_XF | Z80_HF | Z80_NF | Z80_PF));
  if (result & 0x80) f |= Z80_SF;
  if (result == 0) f |= Z80_ZF;
  f |= (uint8_t)(result & (Z80_YF | Z80_XF));
  if (z80_parity(result)) f |= Z80_PF;
  z80_set_f(cpu, f);
}

static void z80_cb_store(z80_t *cpu, uint8_t v) {
  if (cpu->dst.kind == Z80_OP_MEM_HL) {
    z80_mem_wr(cpu, z80_ea_hl_idx(cpu), v);
  } else {
    z80_set_r8(cpu, cpu->dst.reg, v);
  }
}

static uint8_t z80_cb_load(z80_t *cpu) {
  if (cpu->src.kind == Z80_OP_MEM_HL) {
    return cpu->tmp8;
  }
  return z80_get_r8(cpu, cpu->src.reg);
}

static void z80_cb_apply(z80_t *cpu, uint8_t (*op)(uint8_t v, uint8_t cin, uint8_t *cout)) {
  if (cpu->src.kind == Z80_OP_MEM_HL) {
    switch (cpu->m_index) {
    case 1:
      cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
      if (cpu->m_count > 1) {
        z80_cb_advance(cpu);
        return;
      }
      /* fall through for single-shot mem path */
      break;
    case 2:
      cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
      z80_cb_advance(cpu);
      return;
    case 3:
      break;
    default:
      z80_cb_finish(cpu);
      return;
    }
  }

  uint8_t cin = (uint8_t)(z80_get_f(cpu) & Z80_CF);
  uint8_t cout = 0;
  uint8_t v = z80_cb_load(cpu);
  uint8_t result = op(v, cin, &cout);
  z80_cb_set_flags(cpu, result, cout);
  z80_cb_store(cpu, result);
  z80_cb_finish(cpu);
}

static uint8_t cb_rlc(uint8_t v, uint8_t cin, uint8_t *cout) {
  (void)cin;
  *cout = (uint8_t)((v >> 7) & 1);
  return (uint8_t)((v << 1) | *cout);
}

static uint8_t cb_rrc(uint8_t v, uint8_t cin, uint8_t *cout) {
  (void)cin;
  *cout = (uint8_t)(v & 1);
  return (uint8_t)((v >> 1) | (uint8_t)(*cout << 7));
}

static uint8_t cb_rl(uint8_t v, uint8_t cin, uint8_t *cout) {
  *cout = (uint8_t)((v >> 7) & 1);
  return (uint8_t)((v << 1) | (cin & 1));
}

static uint8_t cb_rr(uint8_t v, uint8_t cin, uint8_t *cout) {
  *cout = (uint8_t)(v & 1);
  return (uint8_t)((v >> 1) | (uint8_t)((cin & 1) << 7));
}

static uint8_t cb_sla(uint8_t v, uint8_t cin, uint8_t *cout) {
  (void)cin;
  *cout = (uint8_t)((v >> 7) & 1);
  return (uint8_t)(v << 1);
}

static uint8_t cb_sra(uint8_t v, uint8_t cin, uint8_t *cout) {
  (void)cin;
  *cout = (uint8_t)(v & 1);
  return (uint8_t)((v >> 1) | (uint8_t)(v & 0x80));
}

static uint8_t cb_srl(uint8_t v, uint8_t cin, uint8_t *cout) {
  (void)cin;
  *cout = (uint8_t)(v & 1);
  return (uint8_t)(v >> 1);
}

void z80_m_cb_rlc(z80_t *cpu) { z80_cb_apply(cpu, cb_rlc); }

void z80_m_cb_rrc(z80_t *cpu) { z80_cb_apply(cpu, cb_rrc); }

void z80_m_cb_rl(z80_t *cpu) { z80_cb_apply(cpu, cb_rl); }

void z80_m_cb_rr(z80_t *cpu) { z80_cb_apply(cpu, cb_rr); }

void z80_m_cb_sla(z80_t *cpu) { z80_cb_apply(cpu, cb_sla); }

void z80_m_cb_sra(z80_t *cpu) { z80_cb_apply(cpu, cb_sra); }

void z80_m_cb_srl(z80_t *cpu) { z80_cb_apply(cpu, cb_srl); }

void z80_m_cb_bit(z80_t *cpu) {
  if (cpu->src.kind == Z80_OP_MEM_HL) {
    switch (cpu->m_index) {
    case 1:
      cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
      if (cpu->m_count > 1) {
        z80_cb_advance(cpu);
        return;
      }
      break;
    case 2:
      cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
      z80_cb_advance(cpu);
      return;
    case 3:
      break;
    default:
      z80_cb_finish(cpu);
      return;
    }
  }

  uint8_t v = z80_cb_load(cpu);
  uint8_t mask = (uint8_t)(1U << cpu->bit);
  uint8_t f = z80_get_f(cpu);
  f = (uint8_t)(f & Z80_CF);
  f = (uint8_t)(f & (uint8_t)~(Z80_SF | Z80_ZF | Z80_YF | Z80_XF | Z80_HF | Z80_NF | Z80_PF));
  f |= Z80_HF | Z80_NF;
  if ((v & mask) == 0) f |= Z80_ZF;
  f |= (uint8_t)(v & (Z80_YF | Z80_XF));
  z80_set_f(cpu, f);
  z80_cb_finish(cpu);
}

static void z80_cb_bitop(z80_t *cpu, bool set) {
  if (cpu->src.kind == Z80_OP_MEM_HL) {
    switch (cpu->m_index) {
    case 1:
      cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
      if (cpu->m_count > 1) {
        z80_cb_advance(cpu);
        return;
      }
      break;
    case 2:
      cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
      z80_cb_advance(cpu);
      return;
    case 3:
      break;
    default:
      z80_cb_finish(cpu);
      return;
    }
  }

  uint8_t v = z80_cb_load(cpu);
  uint8_t mask = (uint8_t)(1U << cpu->bit);
  if (set) {
    v = (uint8_t)(v | mask);
  } else {
    v = (uint8_t)(v & (uint8_t)~mask);
  }
  z80_cb_store(cpu, v);
  z80_cb_finish(cpu);
}

void z80_m_cb_res(z80_t *cpu) { z80_cb_bitop(cpu, false); }

void z80_m_cb_set(z80_t *cpu) { z80_cb_bitop(cpu, true); }

void z80_m_cb_undoc(z80_t *cpu) {
  Z80_UNDOC_HIT(cpu);
  z80_cb_finish(cpu);
}
