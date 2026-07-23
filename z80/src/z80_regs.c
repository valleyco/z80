#include "z80_internal.h"

void z80_insn_done(z80_t *cpu) {
  cpu->index = Z80_IDX_NONE;
  cpu->cb_after_index = false;
  cpu->block_repeat = false;
  cpu->phase = Z80_PH_M1;
}

uint8_t z80_mem_rd(z80_t *cpu, uint16_t addr) {
  return cpu->bus.mem_read(cpu->bus.bus_ctx, addr);
}

void z80_mem_wr(z80_t *cpu, uint16_t addr, uint8_t v) {
  cpu->bus.mem_write(cpu->bus.bus_ctx, addr, v);
}

uint8_t z80_io_rd(z80_t *cpu, uint16_t port) {
  return cpu->bus.io_read(cpu->bus.bus_ctx, port);
}

void z80_io_wr(z80_t *cpu, uint16_t port, uint8_t v) {
  cpu->bus.io_write(cpu->bus.bus_ctx, port, v);
}

uint8_t z80_get_f(const z80_t *cpu) { return (uint8_t)(cpu->af & 0xFF); }
void z80_set_f(z80_t *cpu, uint8_t f) { cpu->af = (cpu->af & 0xFF00) | f; }
uint8_t z80_get_a(const z80_t *cpu) { return (uint8_t)(cpu->af >> 8); }
void z80_set_a(z80_t *cpu, uint8_t a) { cpu->af = (uint16_t)((a << 8) | (cpu->af & 0xFF)); }

uint8_t z80_parity(uint8_t v) {
  v ^= (uint8_t)(v >> 4);
  v ^= (uint8_t)(v >> 2);
  v ^= (uint8_t)(v >> 1);
  return (uint8_t)((~v) & 1);
}

void z80_set_sz53p(z80_t *cpu, uint8_t v) {
  uint8_t f = z80_get_f(cpu);
  f &= (uint8_t)(Z80_CF | Z80_HF); /* caller manages H/C separately often — keep C, clear N */
  f &= (uint8_t)~(Z80_SF | Z80_ZF | Z80_YF | Z80_XF | Z80_PF | Z80_NF);
  if (v & 0x80) f |= Z80_SF;
  if (v == 0) f |= Z80_ZF;
  f |= (uint8_t)(v & (Z80_YF | Z80_XF));
  if (z80_parity(v)) f |= Z80_PF;
  z80_set_f(cpu, f);
}

uint16_t z80_hl_or_ixiy(const z80_t *cpu) {
  if (cpu->index == Z80_IDX_IX) return cpu->ix;
  if (cpu->index == Z80_IDX_IY) return cpu->iy;
  return cpu->hl;
}

void z80_set_hl_or_ixiy(z80_t *cpu, uint16_t v) {
  if (cpu->index == Z80_IDX_IX) cpu->ix = v;
  else if (cpu->index == Z80_IDX_IY) cpu->iy = v;
  else cpu->hl = v;
}

uint16_t z80_ea_hl_idx(z80_t *cpu) {
  if (cpu->index == Z80_IDX_NONE) return cpu->hl;
  return (uint16_t)(z80_hl_or_ixiy(cpu) + (int16_t)cpu->disp);
}

uint8_t z80_get_r8(z80_t *cpu, uint8_t r) {
  switch (r & 7) {
  case 0: return (uint8_t)(cpu->bc >> 8);
  case 1: return (uint8_t)(cpu->bc & 0xFF);
  case 2: return (uint8_t)(cpu->de >> 8);
  case 3: return (uint8_t)(cpu->de & 0xFF);
  case 4: /* H / IXH / IYH */
    if (cpu->index == Z80_IDX_IX) return (uint8_t)(cpu->ix >> 8);
    if (cpu->index == Z80_IDX_IY) return (uint8_t)(cpu->iy >> 8);
    return (uint8_t)(cpu->hl >> 8);
  case 5: /* L / IXL / IYL */
    if (cpu->index == Z80_IDX_IX) return (uint8_t)(cpu->ix & 0xFF);
    if (cpu->index == Z80_IDX_IY) return (uint8_t)(cpu->iy & 0xFF);
    return (uint8_t)(cpu->hl & 0xFF);
  case 6: return z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
  case 7: return z80_get_a(cpu);
  default: return 0;
  }
}

void z80_set_r8(z80_t *cpu, uint8_t r, uint8_t v) {
  switch (r & 7) {
  case 0: cpu->bc = (uint16_t)((v << 8) | (cpu->bc & 0xFF)); break;
  case 1: cpu->bc = (uint16_t)((cpu->bc & 0xFF00) | v); break;
  case 2: cpu->de = (uint16_t)((v << 8) | (cpu->de & 0xFF)); break;
  case 3: cpu->de = (uint16_t)((cpu->de & 0xFF00) | v); break;
  case 4:
    if (cpu->index == Z80_IDX_IX) cpu->ix = (uint16_t)((v << 8) | (cpu->ix & 0xFF));
    else if (cpu->index == Z80_IDX_IY) cpu->iy = (uint16_t)((v << 8) | (cpu->iy & 0xFF));
    else cpu->hl = (uint16_t)((v << 8) | (cpu->hl & 0xFF));
    break;
  case 5:
    if (cpu->index == Z80_IDX_IX) cpu->ix = (uint16_t)((cpu->ix & 0xFF00) | v);
    else if (cpu->index == Z80_IDX_IY) cpu->iy = (uint16_t)((cpu->iy & 0xFF00) | v);
    else cpu->hl = (uint16_t)((cpu->hl & 0xFF00) | v);
    break;
  case 6: z80_mem_wr(cpu, z80_ea_hl_idx(cpu), v); break;
  case 7: z80_set_a(cpu, v); break;
  }
}

uint16_t z80_get_rp(z80_t *cpu, uint8_t rp) {
  switch (rp & 3) {
  case 0: return cpu->bc;
  case 1: return cpu->de;
  case 2: return z80_hl_or_ixiy(cpu);
  default: return cpu->sp;
  }
}

void z80_set_rp(z80_t *cpu, uint8_t rp, uint16_t v) {
  switch (rp & 3) {
  case 0: cpu->bc = v; break;
  case 1: cpu->de = v; break;
  case 2: z80_set_hl_or_ixiy(cpu, v); break;
  default: cpu->sp = v; break;
  }
}

uint16_t z80_get_rp_af(z80_t *cpu, uint8_t rp) {
  switch (rp & 3) {
  case 0: return cpu->bc;
  case 1: return cpu->de;
  case 2: return z80_hl_or_ixiy(cpu);
  default: return cpu->af;
  }
}

void z80_set_rp_af(z80_t *cpu, uint8_t rp, uint16_t v) {
  switch (rp & 3) {
  case 0: cpu->bc = v; break;
  case 1: cpu->de = v; break;
  case 2: z80_set_hl_or_ixiy(cpu, v); break;
  default: cpu->af = v; break;
  }
}

bool z80_eval_cc(const z80_t *cpu, uint8_t cc) {
  uint8_t f = z80_get_f(cpu);
  switch (cc & 7) {
  case 0: return (f & Z80_ZF) == 0; /* NZ */
  case 1: return (f & Z80_ZF) != 0; /* Z */
  case 2: return (f & Z80_CF) == 0; /* NC */
  case 3: return (f & Z80_CF) != 0; /* C */
  case 4: return (f & Z80_PF) == 0; /* PO */
  case 5: return (f & Z80_PF) != 0; /* PE */
  case 6: return (f & Z80_SF) == 0; /* P */
  default: return (f & Z80_SF) != 0; /* M */
  }
}

bool z80_eval_cc_jr(const z80_t *cpu, uint8_t cc) {
  return z80_eval_cc(cpu, (uint8_t)(cc & 3)); /* NZ Z NC C */
}

void z80_alu8(z80_t *cpu, uint8_t op, uint8_t rhs) {
  uint8_t a = z80_get_a(cpu);
  uint8_t f = z80_get_f(cpu);
  uint16_t res;
  uint8_t c = (uint8_t)(f & Z80_CF);

  switch (op & 7) {
  case 0: /* ADD */
    res = (uint16_t)(a + rhs);
    f = 0;
    if (((a & 0x0F) + (rhs & 0x0F)) > 0x0F) f |= Z80_HF;
    if (((a ^ rhs ^ res) & 0x80) == 0 && ((a ^ res) & 0x80)) f |= Z80_PF; /* overflow */
    if (res & 0x100) f |= Z80_CF;
    a = (uint8_t)res;
    break;
  case 1: /* ADC */
    res = (uint16_t)(a + rhs + c);
    f = 0;
    if (((a & 0x0F) + (rhs & 0x0F) + c) > 0x0F) f |= Z80_HF;
    if (((a ^ rhs ^ 0x80) & (a ^ res) & 0x80)) f |= Z80_PF;
    if (res & 0x100) f |= Z80_CF;
    a = (uint8_t)res;
    break;
  case 2: /* SUB */
  case 7: /* CP */
    res = (uint16_t)(a - rhs);
    f = Z80_NF;
    if (((a & 0x0F) - (rhs & 0x0F)) & 0x10) f |= Z80_HF;
    if (((a ^ rhs) & (a ^ res) & 0x80)) f |= Z80_PF;
    if (res & 0x100) f |= Z80_CF;
    if ((op & 7) != 7) a = (uint8_t)res;
    else {
      /* CP: don't store A, still set SZ53 from result */
      uint8_t r8 = (uint8_t)res;
      if (r8 & 0x80) f |= Z80_SF;
      if (r8 == 0) f |= Z80_ZF;
      f |= (uint8_t)(rhs & (Z80_YF | Z80_XF)); /* undocumented: XY from operand */
      z80_set_f(cpu, f);
      return;
    }
    break;
  case 3: /* SBC */
    res = (uint16_t)(a - rhs - c);
    f = Z80_NF;
    if (((a & 0x0F) - (rhs & 0x0F) - c) & 0x10) f |= Z80_HF;
    if (((a ^ rhs) & (a ^ res) & 0x80)) f |= Z80_PF;
    if (res & 0x100) f |= Z80_CF;
    a = (uint8_t)res;
    break;
  case 4: /* AND */
    a = (uint8_t)(a & rhs);
    f = Z80_HF;
    break;
  case 5: /* XOR */
    a = (uint8_t)(a ^ rhs);
    f = 0;
    break;
  case 6: /* OR */
    a = (uint8_t)(a | rhs);
    f = 0;
    break;
  default:
    return;
  }

  if (a & 0x80) f |= Z80_SF;
  if (a == 0) f |= Z80_ZF;
  f |= (uint8_t)(a & (Z80_YF | Z80_XF));
  if ((op & 7) >= 4 && (op & 7) <= 6) {
    if (z80_parity(a)) f |= Z80_PF;
  }
  z80_set_a(cpu, a);
  z80_set_f(cpu, f);
}

static z80_opnd_t op_reg8(uint8_t r) {
  z80_opnd_t o = {Z80_OP_NONE, 0};
  if ((r & 7) == 6) {
    o.kind = Z80_OP_MEM_HL;
    o.reg = 6;
  } else {
    o.kind = Z80_OP_REG8;
    o.reg = (uint8_t)(r & 7);
  }
  return o;
}

void z80_bind_from_decode(z80_t *cpu, const z80_decode_ent *ent, uint8_t opcode) {
  cpu->opcode = opcode;
  cpu->family = (z80_family_t)ent->family;
  cpu->dst.kind = Z80_OP_NONE;
  cpu->src.kind = Z80_OP_NONE;
  cpu->alu_op = 0;
  cpu->cc = 0;
  cpu->bit = 0;
  cpu->cond_true = true;
  cpu->block_repeat = false;

  switch (cpu->family) {
  case FAM_ED_LDIR:
  case FAM_ED_LDDR:
  case FAM_ED_CPIR:
  case FAM_ED_CPDR:
  case FAM_ED_INIR:
  case FAM_ED_INDR:
  case FAM_ED_OTIR:
  case FAM_ED_OTDR:
    cpu->block_repeat = true;
    break;
  default:
    break;
  }

  switch (ent->bind) {
  case Z80_BIND_LD_R_R:
    cpu->dst = op_reg8((uint8_t)((opcode >> 3) & 7));
    cpu->src = op_reg8((uint8_t)(opcode & 7));
    break;
  case Z80_BIND_ALU_A_R:
    cpu->alu_op = (uint8_t)((opcode >> 3) & 7);
    cpu->src = op_reg8((uint8_t)(opcode & 7));
    cpu->dst = (z80_opnd_t){Z80_OP_REG8, 7};
    break;
  case Z80_BIND_ALU_A_N:
    cpu->alu_op = (uint8_t)((opcode >> 3) & 7);
    cpu->src = (z80_opnd_t){Z80_OP_IMM8, 0};
    cpu->dst = (z80_opnd_t){Z80_OP_REG8, 7};
    break;
  case Z80_BIND_RP:
    cpu->dst = (z80_opnd_t){Z80_OP_REG16, (uint8_t)((opcode >> 4) & 3)};
    cpu->src = cpu->dst;
    break;
  case Z80_BIND_RP_BCDE:
    /* LD A,(BC/DE) and LD (BC/DE),A — bind both so handlers may use src or dst */
    cpu->dst = (z80_opnd_t){Z80_OP_REG16, (uint8_t)((opcode >> 4) & 1)}; /* BC or DE only */
    cpu->src = cpu->dst;
    break;
  case Z80_BIND_R8_DDD:
    cpu->dst = op_reg8((uint8_t)((opcode >> 3) & 7));
    break;
  case Z80_BIND_R8_SSS:
    cpu->src = op_reg8((uint8_t)(opcode & 7));
    break;
  case Z80_BIND_CC:
    cpu->cc = (uint8_t)((opcode >> 3) & 7);
    cpu->cond_true = z80_eval_cc(cpu, cpu->cc);
    break;
  case Z80_BIND_CC_JR:
    cpu->cc = (uint8_t)((opcode >> 3) & 3);
    cpu->cond_true = z80_eval_cc_jr(cpu, cpu->cc);
    break;
  case Z80_BIND_RST:
    cpu->tmp8 = (uint8_t)(opcode & 0x38);
    break;
  case Z80_BIND_CB_SSS:
    cpu->src = op_reg8((uint8_t)(opcode & 7));
    cpu->dst = cpu->src;
    break;
  case Z80_BIND_CB_BIT:
    cpu->bit = (uint8_t)((opcode >> 3) & 7);
    cpu->src = op_reg8((uint8_t)(opcode & 7));
    cpu->dst = cpu->src;
    break;
  case Z80_BIND_ED_DDD:
    cpu->dst = op_reg8((uint8_t)((opcode >> 3) & 7));
    break;
  case Z80_BIND_ED_SSS:
    /* OUT (C),sss encodes SSS in bits 5–3 (same field position as IN's DDD) */
    cpu->src = op_reg8((uint8_t)((opcode >> 3) & 7));
    break;
  case Z80_BIND_ED_RP:
    cpu->dst = (z80_opnd_t){Z80_OP_REG16, (uint8_t)((opcode >> 4) & 3)};
    break;
  case Z80_BIND_IM: {
    uint8_t nn = (uint8_t)((opcode >> 3) & 3);
    cpu->tmp8 = (nn == 0) ? 0 : (nn == 2) ? 1 : (nn == 3) ? 2 : 0;
    break;
  }
  default:
    break;
  }

  /* Adjust m_count for mem operands / conditionals */
  cpu->m_count = (uint8_t)(1 + ent->m_extra);
  if (cpu->family == FAM_LD_R_R) {
    if (cpu->dst.kind == Z80_OP_MEM_HL || cpu->src.kind == Z80_OP_MEM_HL)
      cpu->m_count = 2;
    else
      cpu->m_count = 1;
  } else if (cpu->family == FAM_ALU_A_R) {
    cpu->m_count = (cpu->src.kind == Z80_OP_MEM_HL) ? 2 : 1;
  } else if (cpu->family == FAM_INC_R || cpu->family == FAM_DEC_R) {
    cpu->m_count = (cpu->dst.kind == Z80_OP_MEM_HL) ? 3 : 1;
  } else if (cpu->family == FAM_LD_R_N) {
    /* LD r,n is 2M; LD (HL),n needs a third M to store. */
    cpu->m_count = (cpu->dst.kind == Z80_OP_MEM_HL) ? 3 : 2;
  } else if (cpu->family == FAM_JR_CC || cpu->family == FAM_DJNZ) {
    /* finalized after offset read in EXEC */
  } else if (cpu->family == FAM_RET_CC) {
    cpu->m_count = cpu->cond_true ? 3 : 1;
  } else if (cpu->family == FAM_JP_CC) {
    /* Always fetch both address bytes (taken or not). */
    cpu->m_count = 3;
  } else if (cpu->family == FAM_CALL_CC) {
    /* address still fetched; call skips push if false */
  }
}
