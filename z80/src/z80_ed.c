#include "z80_internal.h"
#include "generated/z80_handlers.h"

static void z80_ed_finish(z80_t *cpu) { z80_insn_done(cpu); }

static void z80_ed_advance(z80_t *cpu) { cpu->m_index++; }

static void z80_ed_block_ld_flags(z80_t *cpu) {
  uint8_t f = z80_get_f(cpu);
  f = (uint8_t)(f & Z80_CF);
  f = (uint8_t)(f & (uint8_t)~(Z80_HF | Z80_NF | Z80_PF));
  if (cpu->bc != 0) f |= Z80_PF;
  z80_set_f(cpu, f);
}

static void z80_ed_set_sz53p_keep_c(z80_t *cpu, uint8_t v) {
  uint8_t f = (uint8_t)(z80_get_f(cpu) & Z80_CF);
  if (v & 0x80) f |= Z80_SF;
  if (v == 0) f |= Z80_ZF;
  f |= (uint8_t)(v & (Z80_YF | Z80_XF));
  if (z80_parity(v)) f |= Z80_PF;
  z80_set_f(cpu, f);
}

static void z80_ed_ldi_unit(z80_t *cpu, bool decrement) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp8 = z80_mem_rd(cpu, cpu->hl);
    cpu->m_index = 3;
    return;
  case 3:
    z80_mem_wr(cpu, cpu->de, cpu->tmp8);
    cpu->m_index = 4;
    return;
  case 4: {
    if (decrement) {
      cpu->hl = (uint16_t)(cpu->hl - 1);
      cpu->de = (uint16_t)(cpu->de - 1);
    } else {
      cpu->hl = (uint16_t)(cpu->hl + 1);
      cpu->de = (uint16_t)(cpu->de + 1);
    }
    cpu->bc = (uint16_t)(cpu->bc - 1);
    z80_ed_block_ld_flags(cpu);
    if (cpu->block_repeat && cpu->bc != 0) {
      cpu->m_index = 2;
      return;
    }
    z80_ed_finish(cpu);
    return;
  }
  default:
    z80_ed_finish(cpu);
    return;
  }
}

static void z80_ed_cpi_unit(z80_t *cpu, bool decrement) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp8 = z80_mem_rd(cpu, cpu->hl);
    cpu->m_index = 3;
    return;
  case 3: {
    uint8_t a = z80_get_a(cpu);
    uint16_t res = (uint16_t)(a - cpu->tmp8);
    uint8_t r8 = (uint8_t)res;
    uint8_t f = (uint8_t)((z80_get_f(cpu) & Z80_CF) | Z80_NF);
    if (((a & 0x0F) - (cpu->tmp8 & 0x0F)) & 0x10) f |= Z80_HF;
    if (r8 & 0x80) f |= Z80_SF;
    if (r8 == 0) f |= Z80_ZF;
    f |= (uint8_t)(r8 & (Z80_YF | Z80_XF));
    if (decrement) cpu->hl = (uint16_t)(cpu->hl - 1);
    else cpu->hl = (uint16_t)(cpu->hl + 1);
    cpu->bc = (uint16_t)(cpu->bc - 1);
    if (cpu->bc != 0) f |= Z80_PF;
    z80_set_f(cpu, f);
    cpu->m_index = 4;
    return;
  }
  case 4:
    /* Internal / repeat decision */
    if (cpu->block_repeat && cpu->bc != 0 && !(z80_get_f(cpu) & Z80_ZF)) {
      cpu->m_index = 2;
      return;
    }
    z80_ed_finish(cpu);
    return;
  default:
    z80_ed_finish(cpu);
    return;
  }
}

static void z80_ed_ini_unit(z80_t *cpu, bool decrement) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp8 = z80_io_rd(cpu, cpu->bc);
    cpu->m_index = 3;
    return;
  case 3:
    z80_mem_wr(cpu, cpu->hl, cpu->tmp8);
    cpu->m_index = 4;
    return;
  case 4: {
    uint8_t b = (uint8_t)((cpu->bc >> 8) - 1);
    cpu->bc = (uint16_t)((b << 8) | (cpu->bc & 0xFF));
    if (decrement) cpu->hl = (uint16_t)(cpu->hl - 1);
    else cpu->hl = (uint16_t)(cpu->hl + 1);
    {
      uint8_t f = (uint8_t)((z80_get_f(cpu) & Z80_CF) | Z80_NF);
      if (b == 0) f |= Z80_ZF;
      if (b & 0x80) f |= Z80_SF;
      f |= (uint8_t)(b & (Z80_YF | Z80_XF));
      z80_set_f(cpu, f);
    }
    if (cpu->block_repeat && b != 0) {
      cpu->m_index = 2;
      return;
    }
    z80_ed_finish(cpu);
    return;
  }
  default:
    z80_ed_finish(cpu);
    return;
  }
}

static void z80_ed_outi_unit(z80_t *cpu, bool decrement) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp8 = z80_mem_rd(cpu, cpu->hl);
    cpu->m_index = 3;
    return;
  case 3: {
    uint8_t b = (uint8_t)((cpu->bc >> 8) - 1);
    cpu->bc = (uint16_t)((b << 8) | (cpu->bc & 0xFF));
    z80_io_wr(cpu, cpu->bc, cpu->tmp8);
    if (decrement) cpu->hl = (uint16_t)(cpu->hl - 1);
    else cpu->hl = (uint16_t)(cpu->hl + 1);
    {
      uint8_t f = (uint8_t)((z80_get_f(cpu) & Z80_CF) | Z80_NF);
      if (b == 0) f |= Z80_ZF;
      if (b & 0x80) f |= Z80_SF;
      f |= (uint8_t)(b & (Z80_YF | Z80_XF));
      z80_set_f(cpu, f);
    }
    cpu->m_index = 4;
    return;
  }
  case 4:
    if (cpu->block_repeat && ((cpu->bc >> 8) & 0xFF) != 0) {
      cpu->m_index = 2;
      return;
    }
    z80_ed_finish(cpu);
    return;
  default:
    z80_ed_finish(cpu);
    return;
  }
}

static void z80_ed_adc_sbc_hl(z80_t *cpu, bool adc) {
  /* ED 16-bit add/sub with carry: several internal Ms; apply on last */
  if (cpu->m_index == 1 || cpu->m_index >= cpu->m_count || cpu->m_index == 4) {
    uint16_t hl = z80_hl_or_ixiy(cpu);
    uint16_t rp = z80_get_rp(cpu, cpu->dst.reg);
    uint8_t c = (uint8_t)(z80_get_f(cpu) & Z80_CF);
    uint32_t res;
    uint8_t f;

    if (adc) {
      res = (uint32_t)hl + rp + c;
      f = 0;
      if (((hl & 0x0FFFU) + (rp & 0x0FFFU) + c) > 0x0FFFU) f |= Z80_HF;
      if (((hl ^ rp ^ 0x8000) & (hl ^ res) & 0x8000)) f |= Z80_PF; /* overflow */
      if (res > 0xFFFFU) f |= Z80_CF;
    } else {
      res = (uint32_t)hl - rp - c;
      f = Z80_NF;
      if (((hl & 0x0FFFU) - (rp & 0x0FFFU) - c) & 0x1000) f |= Z80_HF;
      if (((hl ^ rp) & (hl ^ res) & 0x8000)) f |= Z80_PF;
      if (res & 0x10000U) f |= Z80_CF;
    }

    {
      uint16_t r16 = (uint16_t)res;
      if (r16 & 0x8000) f |= Z80_SF;
      if (r16 == 0) f |= Z80_ZF;
      f |= (uint8_t)((r16 >> 8) & (Z80_YF | Z80_XF));
      z80_set_hl_or_ixiy(cpu, r16);
      z80_set_f(cpu, f);
    }
    z80_ed_finish(cpu);
    return;
  }
  z80_ed_advance(cpu);
}

void z80_m_ed_ldi(z80_t *cpu) { z80_ed_ldi_unit(cpu, false); }
void z80_m_ed_ldir(z80_t *cpu) { z80_ed_ldi_unit(cpu, false); }
void z80_m_ed_ldd(z80_t *cpu) { z80_ed_ldi_unit(cpu, true); }
void z80_m_ed_lddr(z80_t *cpu) { z80_ed_ldi_unit(cpu, true); }

void z80_m_ed_cpi(z80_t *cpu) { z80_ed_cpi_unit(cpu, false); }
void z80_m_ed_cpir(z80_t *cpu) { z80_ed_cpi_unit(cpu, false); }
void z80_m_ed_cpd(z80_t *cpu) { z80_ed_cpi_unit(cpu, true); }
void z80_m_ed_cpdr(z80_t *cpu) { z80_ed_cpi_unit(cpu, true); }

void z80_m_ed_ini(z80_t *cpu) { z80_ed_ini_unit(cpu, false); }
void z80_m_ed_inir(z80_t *cpu) { z80_ed_ini_unit(cpu, false); }
void z80_m_ed_ind(z80_t *cpu) { z80_ed_ini_unit(cpu, true); }
void z80_m_ed_indr(z80_t *cpu) { z80_ed_ini_unit(cpu, true); }

void z80_m_ed_outi(z80_t *cpu) { z80_ed_outi_unit(cpu, false); }
void z80_m_ed_otir(z80_t *cpu) { z80_ed_outi_unit(cpu, false); }
void z80_m_ed_outd(z80_t *cpu) { z80_ed_outi_unit(cpu, true); }
void z80_m_ed_otdr(z80_t *cpu) { z80_ed_outi_unit(cpu, true); }

void z80_m_ed_adc_hl(z80_t *cpu) { z80_ed_adc_sbc_hl(cpu, true); }
void z80_m_ed_sbc_hl(z80_t *cpu) { z80_ed_adc_sbc_hl(cpu, false); }

void z80_m_ed_in_r_c(z80_t *cpu) {
  if (cpu->dst.kind == Z80_OP_MEM_HL) {
    Z80_UNDOC_HIT(cpu);
    z80_ed_finish(cpu);
    return;
  }
  if (cpu->m_index == 1 || cpu->m_index == 2) {
    cpu->tmp8 = z80_io_rd(cpu, cpu->bc);
    z80_set_r8(cpu, cpu->dst.reg, cpu->tmp8);
    z80_ed_set_sz53p_keep_c(cpu, cpu->tmp8);
    z80_ed_finish(cpu);
    return;
  }
  z80_ed_finish(cpu);
}

void z80_m_ed_out_c_r(z80_t *cpu) {
  if (cpu->src.kind == Z80_OP_MEM_HL) {
    /* OUT (C),0 undocumented — write 0 */
    if (cpu->m_index == 1 || cpu->m_index == 2) {
      z80_io_wr(cpu, cpu->bc, 0);
      z80_ed_finish(cpu);
      return;
    }
  }
  if (cpu->m_index == 1 || cpu->m_index == 2) {
    z80_io_wr(cpu, cpu->bc, z80_get_r8(cpu, cpu->src.reg));
    z80_ed_finish(cpu);
    return;
  }
  z80_ed_finish(cpu);
}

void z80_m_ed_nop(z80_t *cpu) {
  Z80_UNDOC_HIT(cpu);
  z80_ed_finish(cpu);
}

void z80_m_ed_im(z80_t *cpu) {
  cpu->im = cpu->tmp8;
  z80_ed_finish(cpu);
}

static void z80_ed_ld_a_ir_flags(z80_t *cpu, uint8_t v) {
  z80_set_a(cpu, v);
  uint8_t f = (uint8_t)(z80_get_f(cpu) & Z80_CF);
  if (v & 0x80) f |= Z80_SF;
  if (v == 0) f |= Z80_ZF;
  f |= (uint8_t)(v & (Z80_YF | Z80_XF));
  if (cpu->iff2) f |= Z80_PF;
  z80_set_f(cpu, f);
}

void z80_m_ed_ld_a_i(z80_t *cpu) {
  z80_ed_ld_a_ir_flags(cpu, cpu->i);
  z80_ed_finish(cpu);
}

void z80_m_ed_ld_a_r(z80_t *cpu) {
  z80_ed_ld_a_ir_flags(cpu, cpu->r);
  z80_ed_finish(cpu);
}

void z80_m_ed_ld_i_a(z80_t *cpu) {
  cpu->i = z80_get_a(cpu);
  z80_ed_finish(cpu);
}

void z80_m_ed_ld_r_a(z80_t *cpu) {
  cpu->r = z80_get_a(cpu);
  z80_ed_finish(cpu);
}

void z80_m_ed_neg(z80_t *cpu) {
  uint8_t a = z80_get_a(cpu);
  z80_set_a(cpu, 0);
  z80_alu8(cpu, 2 /* SUB */, a);
  z80_ed_finish(cpu);
}

void z80_m_ed_ld_inn_rp(z80_t *cpu) {
  /* LD (nn),rp */
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_ed_advance(cpu);
    break;
  case 3:
    cpu->addr = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    cpu->tmp16 = z80_get_rp(cpu, cpu->dst.reg);
    z80_mem_wr(cpu, cpu->addr, (uint8_t)(cpu->tmp16 & 0xFF));
    z80_ed_advance(cpu);
    break;
  case 4:
    z80_mem_wr(cpu, (uint16_t)(cpu->addr + 1), (uint8_t)(cpu->tmp16 >> 8));
    z80_ed_finish(cpu);
    break;
  default:
    z80_ed_finish(cpu);
    break;
  }
}

void z80_m_ed_ld_rp_inn(z80_t *cpu) {
  /* LD rp,(nn) */
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->pc++);
    z80_ed_advance(cpu);
    break;
  case 3:
    cpu->addr = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->pc++) << 8));
    cpu->tmp8 = z80_mem_rd(cpu, cpu->addr);
    z80_ed_advance(cpu);
    break;
  case 4:
    cpu->tmp16 = (uint16_t)(cpu->tmp8 | ((uint16_t)z80_mem_rd(cpu, (uint16_t)(cpu->addr + 1)) << 8));
    z80_set_rp(cpu, cpu->dst.reg, cpu->tmp16);
    z80_ed_finish(cpu);
    break;
  default:
    z80_ed_finish(cpu);
    break;
  }
}

static void z80_ed_ret(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->sp++);
    z80_ed_advance(cpu);
    break;
  case 3:
    cpu->pc = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->sp++) << 8));
    z80_ed_finish(cpu);
    break;
  default:
    z80_ed_finish(cpu);
    break;
  }
}

void z80_m_ed_reti(z80_t *cpu) {
  /* V1: same as RET (no interrupt nest tracking) */
  z80_ed_ret(cpu);
}

void z80_m_ed_retn(z80_t *cpu) {
  if (cpu->m_index == 1 || cpu->m_index == 2) {
    cpu->tmp16 = (uint16_t)z80_mem_rd(cpu, cpu->sp++);
    z80_ed_advance(cpu);
    return;
  }
  if (cpu->m_index == 3) {
    cpu->pc = (uint16_t)(cpu->tmp16 | ((uint16_t)z80_mem_rd(cpu, cpu->sp++) << 8));
    cpu->iff1 = cpu->iff2;
    z80_ed_finish(cpu);
    return;
  }
  z80_ed_finish(cpu);
}

void z80_m_ed_rld(z80_t *cpu) {
  /* A[3:0] ← (HL)[7:4]; (HL) ← (HL)<<4 | A[3:0]_old  via rotate left through A low */
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp8 = z80_mem_rd(cpu, cpu->hl);
    z80_ed_advance(cpu);
    break;
  case 3: {
    uint8_t a = z80_get_a(cpu);
    uint8_t mem = cpu->tmp8;
    uint8_t new_mem = (uint8_t)((mem << 4) | (a & 0x0F));
    uint8_t new_a = (uint8_t)((a & 0xF0) | (mem >> 4));
    z80_set_a(cpu, new_a);
    z80_ed_set_sz53p_keep_c(cpu, new_a);
    cpu->tmp8 = new_mem;
    z80_ed_advance(cpu);
    break;
  }
  case 4:
    z80_mem_wr(cpu, cpu->hl, cpu->tmp8);
    z80_ed_finish(cpu);
    break;
  default:
    z80_ed_finish(cpu);
    break;
  }
}

void z80_m_ed_rrd(z80_t *cpu) {
  switch (cpu->m_index) {
  case 1:
  case 2:
    cpu->tmp8 = z80_mem_rd(cpu, cpu->hl);
    z80_ed_advance(cpu);
    break;
  case 3: {
    uint8_t a = z80_get_a(cpu);
    uint8_t mem = cpu->tmp8;
    uint8_t new_mem = (uint8_t)((a << 4) | (mem >> 4));
    uint8_t new_a = (uint8_t)((a & 0xF0) | (mem & 0x0F));
    z80_set_a(cpu, new_a);
    z80_ed_set_sz53p_keep_c(cpu, new_a);
    cpu->tmp8 = new_mem;
    z80_ed_advance(cpu);
    break;
  }
  case 4:
    z80_mem_wr(cpu, cpu->hl, cpu->tmp8);
    z80_ed_finish(cpu);
    break;
  default:
    z80_ed_finish(cpu);
    break;
  }
}
