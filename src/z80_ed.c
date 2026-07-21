#include "z80_internal.h"
#include "generated/z80_handlers.h"

static void z80_ed_finish(z80_t *cpu) { z80_insn_done(cpu); }

static void z80_ed_stub(z80_t *cpu) {
  (void)cpu;
  z80_ed_finish(cpu);
}

static void z80_ed_block_flags(z80_t *cpu) {
  uint8_t f = z80_get_f(cpu);
  f = (uint8_t)(f & Z80_CF);
  f = (uint8_t)(f & (uint8_t)~(Z80_HF | Z80_NF | Z80_PF));
  if (cpu->bc != 0) f |= Z80_PF;
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
    z80_ed_block_flags(cpu);
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

void z80_m_ed_ldi(z80_t *cpu) { z80_ed_ldi_unit(cpu, false); }

void z80_m_ed_ldir(z80_t *cpu) { z80_ed_ldi_unit(cpu, false); }

void z80_m_ed_ldd(z80_t *cpu) { z80_ed_ldi_unit(cpu, true); }

void z80_m_ed_lddr(z80_t *cpu) { z80_ed_ldi_unit(cpu, true); }

void z80_m_ed_in_r_c(z80_t *cpu) {
  if (cpu->dst.kind == Z80_OP_MEM_HL) {
    Z80_UNDOC_HIT(cpu);
    z80_ed_finish(cpu);
    return;
  }

  if (cpu->m_index == 1) {
    cpu->tmp8 = z80_io_rd(cpu, cpu->bc);
    z80_set_r8(cpu, cpu->dst.reg, cpu->tmp8);
    uint8_t f = z80_get_f(cpu);
    f = (uint8_t)(f & Z80_CF);
    f = (uint8_t)(f & (uint8_t)~(Z80_SF | Z80_ZF | Z80_YF | Z80_XF | Z80_HF | Z80_NF | Z80_PF));
    if (cpu->tmp8 & 0x80) f |= Z80_SF;
    if (cpu->tmp8 == 0) f |= Z80_ZF;
    f |= (uint8_t)(cpu->tmp8 & (Z80_YF | Z80_XF));
    if (z80_parity(cpu->tmp8)) f |= Z80_PF;
    z80_set_f(cpu, f);
    z80_ed_finish(cpu);
    return;
  }

  if (cpu->m_index == 2) {
    cpu->tmp8 = z80_io_rd(cpu, cpu->bc);
    z80_set_r8(cpu, cpu->dst.reg, cpu->tmp8);
    uint8_t f = z80_get_f(cpu);
    f = (uint8_t)(f & Z80_CF);
    f = (uint8_t)(f & (uint8_t)~(Z80_SF | Z80_ZF | Z80_YF | Z80_XF | Z80_HF | Z80_NF | Z80_PF));
    if (cpu->tmp8 & 0x80) f |= Z80_SF;
    if (cpu->tmp8 == 0) f |= Z80_ZF;
    f |= (uint8_t)(cpu->tmp8 & (Z80_YF | Z80_XF));
    if (z80_parity(cpu->tmp8)) f |= Z80_PF;
    z80_set_f(cpu, f);
    z80_ed_finish(cpu);
    return;
  }

  z80_ed_finish(cpu);
}

void z80_m_ed_nop(z80_t *cpu) {
  Z80_UNDOC_HIT(cpu);
  z80_ed_finish(cpu);
}

void z80_m_ed_adc_hl(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_cpd(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_cpdr(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_cpi(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_cpir(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_im(z80_t *cpu) {
  cpu->im = cpu->tmp8;
  z80_ed_finish(cpu);
}

void z80_m_ed_ind(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_indr(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_ini(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_inir(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_ld_a_i(z80_t *cpu) {
  z80_set_a(cpu, cpu->i);
  z80_ed_finish(cpu);
}

void z80_m_ed_ld_a_r(z80_t *cpu) {
  z80_set_a(cpu, cpu->r);
  z80_ed_finish(cpu);
}

void z80_m_ed_ld_inn_rp(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_ld_i_a(z80_t *cpu) {
  cpu->i = z80_get_a(cpu);
  z80_ed_finish(cpu);
}

void z80_m_ed_ld_rp_inn(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_ld_r_a(z80_t *cpu) {
  cpu->r = z80_get_a(cpu);
  z80_ed_finish(cpu);
}

void z80_m_ed_neg(z80_t *cpu) {
  z80_alu8(cpu, 7, z80_get_a(cpu));
  z80_ed_finish(cpu);
}

void z80_m_ed_otdr(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_otir(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_outd(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_outi(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_out_c_r(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_reti(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_retn(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_rld(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_rrd(z80_t *cpu) { z80_ed_stub(cpu); }

void z80_m_ed_sbc_hl(z80_t *cpu) { z80_ed_stub(cpu); }
