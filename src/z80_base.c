#include "z80_internal.h"
#include "generated/z80_handlers.h"

static void z80_advance(z80_t *cpu) { cpu->m_index++; }

static void z80_finish(z80_t *cpu) { z80_insn_done(cpu); }

static void z80_step_done(z80_t *cpu) {
  if (cpu->m_index >= cpu->m_count) {
    z80_finish(cpu);
  } else {
    z80_advance(cpu);
  }
}

static void z80_stub(z80_t *cpu) {
  (void)cpu;
  z80_finish(cpu);
}

static void z80_push16(z80_t *cpu, uint16_t v) {
  cpu->sp = (uint16_t)(cpu->sp - 1);
  z80_mem_wr(cpu, cpu->sp, (uint8_t)(v >> 8));
  cpu->sp = (uint16_t)(cpu->sp - 1);
  z80_mem_wr(cpu, cpu->sp, (uint8_t)(v & 0xFF));
}

static void z80_flags_inc(z80_t *cpu, uint8_t v, uint8_t result) {
  uint8_t f = z80_get_f(cpu);
  f = (uint8_t)(f & Z80_CF);
  f = (uint8_t)(f & (uint8_t)~(Z80_SF | Z80_ZF | Z80_YF | Z80_XF | Z80_HF | Z80_NF | Z80_PF));
  if (result & 0x80) f |= Z80_SF;
  if (result == 0) f |= Z80_ZF;
  f |= (uint8_t)(result & (Z80_YF | Z80_XF));
  if ((v & 0x0F) == 0x0F) f |= Z80_HF;
  if (z80_parity(result)) f |= Z80_PF;
  z80_set_f(cpu, f);
}

static void z80_flags_dec(z80_t *cpu, uint8_t v, uint8_t result) {
  uint8_t f = z80_get_f(cpu);
  f = (uint8_t)(f & Z80_CF);
  f = (uint8_t)(f & (uint8_t)~(Z80_SF | Z80_ZF | Z80_YF | Z80_XF | Z80_HF | Z80_NF | Z80_PF));
  f |= Z80_NF;
  if (result & 0x80) f |= Z80_SF;
  if (result == 0) f |= Z80_ZF;
  f |= (uint8_t)(result & (Z80_YF | Z80_XF));
  if ((v & 0x0F) == 0x00) f |= Z80_HF;
  if (z80_parity(result)) f |= Z80_PF;
  z80_set_f(cpu, f);
}

static uint8_t z80_op_reg(const z80_opnd_t *op) { return op->reg; }

static bool z80_op_is_mem(const z80_opnd_t *op) { return op->kind == Z80_OP_MEM_HL; }

void z80_m_nop(z80_t *cpu) { z80_finish(cpu); }

void z80_m_undoc(z80_t *cpu) {
  Z80_UNDOC_HIT(cpu);
  z80_finish(cpu);
}

void z80_m_halt(z80_t *cpu) {
  cpu->halted = true;
  z80_finish(cpu);
}

void z80_m_di(z80_t *cpu) {
  cpu->iff1 = 0;
  cpu->iff2 = 0;
  z80_finish(cpu);
}

void z80_m_ei(z80_t *cpu) {
  cpu->iff1 = 1;
  cpu->iff2 = 1;
  z80_finish(cpu);
}

void z80_m_ld_r_r(z80_t *cpu) {
  const bool dst_mem = z80_op_is_mem(&cpu->dst);
  const bool src_mem = z80_op_is_mem(&cpu->src);

  if (!dst_mem && !src_mem) {
    z80_set_r8(cpu, z80_op_reg(&cpu->dst), z80_get_r8(cpu, z80_op_reg(&cpu->src)));
    z80_finish(cpu);
    return;
  }

  /* Memory operand: one post-M1 cycle does the bus transfer */
  if (src_mem) {
    cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
    if (!dst_mem) {
      z80_set_r8(cpu, z80_op_reg(&cpu->dst), cpu->tmp8);
    } else {
      z80_mem_wr(cpu, z80_ea_hl_idx(cpu), cpu->tmp8); /* LD (HL),(HL) — rare */
    }
    z80_finish(cpu);
    return;
  }

  /* dst mem, src reg */
  cpu->tmp8 = z80_get_r8(cpu, z80_op_reg(&cpu->src));
  z80_mem_wr(cpu, z80_ea_hl_idx(cpu), cpu->tmp8);
  z80_finish(cpu);
}

void z80_m_alu_a_r(z80_t *cpu) {
  if (z80_op_is_mem(&cpu->src)) {
    cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
    z80_alu8(cpu, cpu->alu_op, cpu->tmp8);
    z80_finish(cpu);
    return;
  }

  z80_alu8(cpu, cpu->alu_op, z80_get_r8(cpu, z80_op_reg(&cpu->src)));
  z80_finish(cpu);
}

void z80_m_alu_a_n(z80_t *cpu) {
  if (cpu->m_index == 1) {
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    z80_alu8(cpu, cpu->alu_op, cpu->tmp8);
    z80_finish(cpu);
    return;
  }

  switch (cpu->m_index) {
  case 2:
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    z80_alu8(cpu, cpu->alu_op, cpu->tmp8);
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_ld_rp_nn(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->tmp16 = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    z80_set_rp(cpu, z80_op_reg(&cpu->dst), cpu->tmp16);
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_jp_nn(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->pc = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_jr(z80_t *cpu) {
  if (cpu->m_index == 1) {
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    cpu->pc = (uint16_t)(cpu->pc + (int8_t)cpu->tmp8);
    z80_finish(cpu);
    return;
  }

  if (cpu->m_index == 2) {
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    cpu->pc = (uint16_t)(cpu->pc + (int8_t)cpu->tmp8);
    z80_finish(cpu);
    return;
  }

  z80_finish(cpu);
}

void z80_m_jr_cc(z80_t *cpu) {
  if (cpu->m_index == 1) {
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    if (cpu->cond_true) {
      cpu->pc = (uint16_t)(cpu->pc + (int8_t)cpu->tmp8);
    }
    z80_finish(cpu);
    return;
  }

  if (cpu->m_index == 2) {
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    if (cpu->cond_true) {
      cpu->pc = (uint16_t)(cpu->pc + (int8_t)cpu->tmp8);
    }
    z80_finish(cpu);
    return;
  }

  z80_finish(cpu);
}

void z80_m_ld_r_n(z80_t *cpu) {
  if (z80_op_is_mem(&cpu->dst)) {
    switch (cpu->m_index) {
    case 1:
    case 2:
      cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
      z80_step_done(cpu);
      break;
    case 3:
      z80_mem_wr(cpu, z80_ea_hl_idx(cpu), cpu->tmp8);
      z80_finish(cpu);
      break;
    default:
      z80_finish(cpu);
      break;
    }
    return;
  }

  if (cpu->m_index == 1) {
    z80_set_r8(cpu, z80_op_reg(&cpu->dst), z80_mem_rd(cpu, cpu->pc++));
    z80_finish(cpu);
    return;
  }

  if (cpu->m_index == 2) {
    z80_set_r8(cpu, z80_op_reg(&cpu->dst), z80_mem_rd(cpu, cpu->pc++));
    z80_finish(cpu);
    return;
  }

  z80_finish(cpu);
}

static void z80_inc_dec_r(z80_t *cpu, bool dec) {
  if (z80_op_is_mem(&cpu->dst)) {
    switch (cpu->m_index) {
    case 1:
    case 2:
      cpu->tmp8 = z80_mem_rd(cpu, z80_ea_hl_idx(cpu));
      z80_step_done(cpu);
      break;
    case 3: {
      uint8_t v = cpu->tmp8;
      uint8_t result = dec ? (uint8_t)(v - 1) : (uint8_t)(v + 1);
      if (dec) {
        z80_flags_dec(cpu, v, result);
      } else {
        z80_flags_inc(cpu, v, result);
      }
      z80_mem_wr(cpu, z80_ea_hl_idx(cpu), result);
      z80_finish(cpu);
      break;
    }
    default:
      z80_finish(cpu);
      break;
    }
    return;
  }

  uint8_t v = z80_get_r8(cpu, z80_op_reg(&cpu->dst));
  uint8_t result = dec ? (uint8_t)(v - 1) : (uint8_t)(v + 1);
  if (dec) {
    z80_flags_dec(cpu, v, result);
  } else {
    z80_flags_inc(cpu, v, result);
  }
  z80_set_r8(cpu, z80_op_reg(&cpu->dst), result);
  z80_finish(cpu);
}

void z80_m_inc_r(z80_t *cpu) { z80_inc_dec_r(cpu, false); }

void z80_m_dec_r(z80_t *cpu) { z80_inc_dec_r(cpu, true); }

void z80_m_inc_rp(z80_t *cpu) {
  uint16_t v = z80_get_rp(cpu, z80_op_reg(&cpu->dst));
  z80_set_rp(cpu, z80_op_reg(&cpu->dst), (uint16_t)(v + 1));
  z80_finish(cpu);
}

void z80_m_dec_rp(z80_t *cpu) {
  uint16_t v = z80_get_rp(cpu, z80_op_reg(&cpu->dst));
  z80_set_rp(cpu, z80_op_reg(&cpu->dst), (uint16_t)(v - 1));
  z80_finish(cpu);
}

void z80_m_ret(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->sp++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->pc = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->sp++) << 8));
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_ret_cc(z80_t *cpu) {
  if (!cpu->cond_true) {
    z80_finish(cpu);
    return;
  }
  z80_m_ret(cpu);
}

void z80_m_call_nn(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_advance(cpu);
    break;
  case 3:
    cpu->tmp16 = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    z80_advance(cpu);
    break;
  case 4:
    z80_push16(cpu, cpu->pc);
    z80_advance(cpu);
    break;
  case 5:
    cpu->pc = cpu->tmp16;
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_call_cc(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->tmp16 = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    if (!cpu->cond_true) {
      z80_finish(cpu);
      return;
    }
    z80_advance(cpu);
    break;
  case 4:
    z80_push16(cpu, cpu->pc);
    z80_advance(cpu);
    break;
  case 5:
    cpu->pc = cpu->tmp16;
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_jp_cc(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->tmp16 = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    if (cpu->cond_true) {
      cpu->pc = cpu->tmp16;
    }
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_push_rp(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    z80_advance(cpu);
    break;
  case 3:
    z80_push16(cpu, z80_get_rp_af(cpu, z80_op_reg(&cpu->dst)));
    z80_advance(cpu);
    break;
  case 4:
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_pop_rp(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->sp++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->tmp16 = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->sp++) << 8));
    z80_set_rp_af(cpu, z80_op_reg(&cpu->dst), cpu->tmp16);
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_ex_de_hl(z80_t *cpu) {
  uint16_t de = cpu->de;
  uint16_t hl = z80_hl_or_ixiy(cpu);
  cpu->de = hl;
  z80_set_hl_or_ixiy(cpu, de);
  z80_finish(cpu);
}

void z80_m_ex_af(z80_t *cpu) {
  uint16_t t = cpu->af;
  cpu->af = cpu->af_;
  cpu->af_ = t;
  z80_finish(cpu);
}

void z80_m_exx(z80_t *cpu) {
  uint16_t t;
  t = cpu->bc;
  cpu->bc = cpu->bc_;
  cpu->bc_ = t;
  t = cpu->de;
  cpu->de = cpu->de_;
  cpu->de_ = t;
  t = cpu->hl;
  cpu->hl = cpu->hl_;
  cpu->hl_ = t;
  z80_finish(cpu);
}

void z80_m_jp_hl(z80_t *cpu) {
  cpu->pc = z80_hl_or_ixiy(cpu);
  z80_finish(cpu);
}

void z80_m_ld_sp_hl(z80_t *cpu) {
  cpu->sp = z80_hl_or_ixiy(cpu);
  z80_finish(cpu);
}

void z80_m_scf(z80_t *cpu) {
  uint8_t f = z80_get_f(cpu);
  f = (uint8_t)((f & (Z80_SF | Z80_ZF | Z80_PF)) | Z80_CF);
  f = (uint8_t)(f & (uint8_t)~(Z80_NF | Z80_HF));
  f |= (uint8_t)(z80_get_a(cpu) & (Z80_YF | Z80_XF));
  z80_set_f(cpu, f);
  z80_finish(cpu);
}

void z80_m_ccf(z80_t *cpu) {
  uint8_t f = z80_get_f(cpu);
  if (f & Z80_CF) {
    f = (uint8_t)((f & (Z80_SF | Z80_ZF | Z80_PF)) | Z80_HF);
  } else {
    f = (uint8_t)((f & (Z80_SF | Z80_ZF | Z80_PF)) | Z80_CF);
    f = (uint8_t)(f & (uint8_t)~Z80_HF);
  }
  f = (uint8_t)(f & (uint8_t)~Z80_NF);
  f |= (uint8_t)(z80_get_a(cpu) & (Z80_YF | Z80_XF));
  z80_set_f(cpu, f);
  z80_finish(cpu);
}

void z80_m_cpl(z80_t *cpu) {
  z80_set_a(cpu, (uint8_t)~z80_get_a(cpu));
  uint8_t f = z80_get_f(cpu);
  f = (uint8_t)((f & (Z80_SF | Z80_ZF | Z80_PF | Z80_CF)) | Z80_NF | Z80_HF);
  f |= (uint8_t)(z80_get_a(cpu) & (Z80_YF | Z80_XF));
  z80_set_f(cpu, f);
  z80_finish(cpu);
}

static void z80_rot_acc(z80_t *cpu, bool left, bool carry_in) {
  uint8_t a = z80_get_a(cpu);
  uint8_t f = z80_get_f(cpu);
  uint8_t c = (uint8_t)(f & Z80_CF);
  uint8_t new_c;

  f = (uint8_t)(f & (Z80_SF | Z80_ZF | Z80_PF));
  f = (uint8_t)(f & (uint8_t)~(Z80_NF | Z80_HF | Z80_CF | Z80_YF | Z80_XF));

  if (left) {
    new_c = (uint8_t)((a >> 7) & 1);
    a = carry_in ? (uint8_t)((a << 1) | c) : (uint8_t)((a << 1) | new_c);
  } else {
    new_c = (uint8_t)(a & 1);
    a = carry_in ? (uint8_t)((a >> 1) | (uint8_t)(c << 7)) : (uint8_t)((a >> 1) | (uint8_t)(new_c << 7));
  }

  if (new_c) f |= Z80_CF;
  f |= (uint8_t)(a & (Z80_YF | Z80_XF));
  z80_set_a(cpu, a);
  z80_set_f(cpu, f);
}

void z80_m_rlca(z80_t *cpu) { z80_rot_acc(cpu, true, false); z80_finish(cpu); }

void z80_m_rrca(z80_t *cpu) { z80_rot_acc(cpu, false, false); z80_finish(cpu); }

void z80_m_rla(z80_t *cpu) { z80_rot_acc(cpu, true, true); z80_finish(cpu); }

void z80_m_rra(z80_t *cpu) { z80_rot_acc(cpu, false, true); z80_finish(cpu); }

void z80_m_ld_irp_a(z80_t *cpu) {
  uint16_t addr = z80_get_rp(cpu, z80_op_reg(&cpu->dst));
  if (cpu->m_index == 1) {
    z80_mem_wr(cpu, addr, z80_get_a(cpu));
    z80_finish(cpu);
    return;
  }
  if (cpu->m_index == 2) {
    z80_mem_wr(cpu, addr, z80_get_a(cpu));
    z80_finish(cpu);
    return;
  }
  z80_finish(cpu);
}

void z80_m_ld_a_irp(z80_t *cpu) {
  uint16_t addr = z80_get_rp(cpu, z80_op_reg(&cpu->src));
  if (cpu->m_index == 1) {
    z80_set_a(cpu, z80_mem_rd(cpu, addr));
    z80_finish(cpu);
    return;
  }
  if (cpu->m_index == 2) {
    z80_set_a(cpu, z80_mem_rd(cpu, addr));
    z80_finish(cpu);
    return;
  }
  z80_finish(cpu);
}

void z80_m_rst(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    z80_push16(cpu, cpu->pc);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->pc = cpu->tmp8;
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_djnz(z80_t *cpu) {
  if (cpu->m_index == 1) {
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    cpu->bc = (uint16_t)((cpu->bc & 0x00FF) | ((uint16_t)((uint8_t)(cpu->bc >> 8) - 1) << 8));
    if ((uint8_t)(cpu->bc >> 8) != 0) {
      cpu->pc = (uint16_t)(cpu->pc + (int8_t)cpu->tmp8);
    }
    z80_finish(cpu);
    return;
  }

  if (cpu->m_index == 2) {
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    cpu->bc = (uint16_t)((cpu->bc & 0x00FF) | ((uint16_t)((uint8_t)(cpu->bc >> 8) - 1) << 8));
    if ((uint8_t)(cpu->bc >> 8) != 0) {
      cpu->pc = (uint16_t)(cpu->pc + (int8_t)cpu->tmp8);
    }
    z80_finish(cpu);
    return;
  }

  z80_finish(cpu);
}

void z80_m_ld_a_inn(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->addr = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    z80_set_a(cpu, z80_mem_rd(cpu, cpu->addr));
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_ld_inn_a(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->addr = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    z80_mem_wr(cpu, cpu->addr, z80_get_a(cpu));
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_ld_hl_inn(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->addr = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    cpu->tmp16 = (uint16_t)(z80_mem_rd(cpu, cpu->addr) | ((uint16_t)z80_mem_rd(cpu, (uint16_t)(cpu->addr + 1)) << 8));
    z80_set_hl_or_ixiy(cpu, cpu->tmp16);
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_ld_inn_hl(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    cpu->addr = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    cpu->tmp16 = z80_hl_or_ixiy(cpu);
    z80_mem_wr(cpu, cpu->addr, (uint8_t)(cpu->tmp16 & 0xFF));
    z80_mem_wr(cpu, (uint16_t)(cpu->addr + 1), (uint8_t)(cpu->tmp16 >> 8));
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_in_a_n(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    z80_set_a(cpu, z80_io_rd(cpu, (uint16_t)((z80_get_a(cpu) << 8) | cpu->tmp8)));
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_out_n_a(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp8 = z80_mem_rd(cpu, cpu->pc++);
    z80_step_done(cpu);
    break;
  case 3:
    z80_io_wr(cpu, (uint16_t)((z80_get_a(cpu) << 8) | cpu->tmp8), z80_get_a(cpu));
    z80_finish(cpu);
    break;
  default:
    z80_finish(cpu);
    break;
  }
}

void z80_m_add_hl_rp(z80_t *cpu) {
  if (cpu->m_index == 1 || cpu->m_index >= cpu->m_count) {
    uint16_t hl = z80_hl_or_ixiy(cpu);
    uint16_t rp = z80_get_rp(cpu, z80_op_reg(&cpu->dst));
    uint32_t res = (uint32_t)hl + rp;
    uint8_t f = z80_get_f(cpu);
    f = (uint8_t)(f & (Z80_SF | Z80_ZF | Z80_PF));
    f = (uint8_t)(f & (uint8_t)~Z80_NF);
    if (((hl & 0x0FFFU) + (rp & 0x0FFFU)) > 0x0FFFU) f |= Z80_HF;
    if (res > 0xFFFFU) f |= Z80_CF;
    z80_set_hl_or_ixiy(cpu, (uint16_t)res);
    z80_set_f(cpu, f);
    z80_finish(cpu);
    return;
  }
  z80_advance(cpu);
}

void z80_m_daa(z80_t *cpu) { z80_stub(cpu); }

void z80_m_ex_isp_hl(z80_t *cpu) { z80_stub(cpu); }
