#include <stdio.h>
#include <string.h>

#include "test_vectors.h"
#include "z80.h"

typedef struct {
  uint8_t mem[65536];
  uint8_t io[256];
} bus_t;

static uint8_t mem_read(void *ctx, uint16_t addr) {
  return ((bus_t *)ctx)->mem[addr];
}

static void mem_write(void *ctx, uint16_t addr, uint8_t v) {
  ((bus_t *)ctx)->mem[addr] = v;
}

static uint8_t io_read(void *ctx, uint16_t port) {
  return ((bus_t *)ctx)->io[port & 0xFF];
}

static void io_write(void *ctx, uint16_t port, uint8_t v) {
  ((bus_t *)ctx)->io[port & 0xFF] = v;
}

static void apply_init(z80_t *cpu, const tv_test_t *t) {
  if (t->init_mask & TV_MASK_AF) cpu->af = t->init.af;
  if (t->init_mask & TV_MASK_BC) cpu->bc = t->init.bc;
  if (t->init_mask & TV_MASK_DE) cpu->de = t->init.de;
  if (t->init_mask & TV_MASK_HL) cpu->hl = t->init.hl;
  if (t->init_mask & TV_MASK_IX) cpu->ix = t->init.ix;
  if (t->init_mask & TV_MASK_IY) cpu->iy = t->init.iy;
  if (t->init_mask & TV_MASK_SP) cpu->sp = t->init.sp;
  if (t->init_mask & TV_MASK_PC) cpu->pc = t->init.pc;
  if (t->init_mask & TV_MASK_AF_) cpu->af_ = t->init.af_;
  if (t->init_mask & TV_MASK_BC_) cpu->bc_ = t->init.bc_;
  if (t->init_mask & TV_MASK_DE_) cpu->de_ = t->init.de_;
  if (t->init_mask & TV_MASK_HL_) cpu->hl_ = t->init.hl_;
  if (t->init_mask & TV_MASK_I) cpu->i = t->init.i;
  if (t->init_mask & TV_MASK_R) cpu->r = t->init.r;
  if (t->init_mask & TV_MASK_IM) cpu->im = t->init.im;
  if (t->init_mask & TV_MASK_IFF1) cpu->iff1 = t->init.iff1;
  if (t->init_mask & TV_MASK_IFF2) cpu->iff2 = t->init.iff2;
  if (t->init_mask & TV_MASK_A) cpu->af = (uint16_t)((cpu->af & 0x00FF) | ((t->init.af & 0xFF00)));
  if (t->init_mask & TV_MASK_F) cpu->af = (uint16_t)((cpu->af & 0xFF00) | (t->init.af & 0x00FF));

  /* Half-registers overlay pair registers (after full-pair inits) */
  if (t->init.r8_mask & TV_R8_B)
    cpu->bc = (uint16_t)((cpu->bc & 0x00FF) | (t->init.bc & 0xFF00));
  if (t->init.r8_mask & TV_R8_C)
    cpu->bc = (uint16_t)((cpu->bc & 0xFF00) | (t->init.bc & 0x00FF));
  if (t->init.r8_mask & TV_R8_D)
    cpu->de = (uint16_t)((cpu->de & 0x00FF) | (t->init.de & 0xFF00));
  if (t->init.r8_mask & TV_R8_E)
    cpu->de = (uint16_t)((cpu->de & 0xFF00) | (t->init.de & 0x00FF));
  if (t->init.r8_mask & TV_R8_H)
    cpu->hl = (uint16_t)((cpu->hl & 0x00FF) | (t->init.hl & 0xFF00));
  if (t->init.r8_mask & TV_R8_L)
    cpu->hl = (uint16_t)((cpu->hl & 0xFF00) | (t->init.hl & 0x00FF));
}

static int check_u16(const char *name, const char *field, uint16_t exp, uint16_t got) {
  if (exp != got) {
    fprintf(stderr, "FAIL %s: expect %s=0x%04X got 0x%04X\n", name, field, exp, got);
    return 1;
  }
  return 0;
}

static int check_u8(const char *name, const char *field, uint8_t exp, uint8_t got) {
  if (exp != got) {
    fprintf(stderr, "FAIL %s: expect %s=0x%02X got 0x%02X\n", name, field, exp, got);
    return 1;
  }
  return 0;
}

static int check_expect(const tv_test_t *t, const z80_t *cpu, const bus_t *bus) {
  const char *name = t->name;
  int fail = 0;
  uint32_t m = t->expect_mask;

  if (m & TV_MASK_AF) fail |= check_u16(name, "af", t->expect.af, cpu->af);
  if (m & TV_MASK_BC) fail |= check_u16(name, "bc", t->expect.bc, cpu->bc);
  if (m & TV_MASK_DE) fail |= check_u16(name, "de", t->expect.de, cpu->de);
  if (m & TV_MASK_HL) fail |= check_u16(name, "hl", t->expect.hl, cpu->hl);
  if (m & TV_MASK_IX) fail |= check_u16(name, "ix", t->expect.ix, cpu->ix);
  if (m & TV_MASK_IY) fail |= check_u16(name, "iy", t->expect.iy, cpu->iy);
  if (m & TV_MASK_SP) fail |= check_u16(name, "sp", t->expect.sp, cpu->sp);
  if (m & TV_MASK_PC) fail |= check_u16(name, "pc", t->expect.pc, cpu->pc);
  if (m & TV_MASK_AF_) fail |= check_u16(name, "af_", t->expect.af_, cpu->af_);
  if (m & TV_MASK_BC_) fail |= check_u16(name, "bc_", t->expect.bc_, cpu->bc_);
  if (m & TV_MASK_DE_) fail |= check_u16(name, "de_", t->expect.de_, cpu->de_);
  if (m & TV_MASK_HL_) fail |= check_u16(name, "hl_", t->expect.hl_, cpu->hl_);
  if (m & TV_MASK_I) fail |= check_u8(name, "i", t->expect.i, cpu->i);
  if (m & TV_MASK_R) fail |= check_u8(name, "r", t->expect.r, cpu->r);
  if (m & TV_MASK_IM) fail |= check_u8(name, "im", t->expect.im, cpu->im);
  if (m & TV_MASK_IFF1) fail |= check_u8(name, "iff1", t->expect.iff1, cpu->iff1);
  if (m & TV_MASK_IFF2) fail |= check_u8(name, "iff2", t->expect.iff2, cpu->iff2);
  if (m & TV_MASK_A) fail |= check_u8(name, "a", (uint8_t)(t->expect.af >> 8), (uint8_t)(cpu->af >> 8));
  if (m & TV_MASK_F) fail |= check_u8(name, "f", (uint8_t)(t->expect.af & 0xFF), (uint8_t)(cpu->af & 0xFF));

  if (t->expect.r8_mask & TV_R8_B)
    fail |= check_u8(name, "b", (uint8_t)(t->expect.bc >> 8), (uint8_t)(cpu->bc >> 8));
  if (t->expect.r8_mask & TV_R8_C)
    fail |= check_u8(name, "c", (uint8_t)(t->expect.bc & 0xFF), (uint8_t)(cpu->bc & 0xFF));
  if (t->expect.r8_mask & TV_R8_D)
    fail |= check_u8(name, "d", (uint8_t)(t->expect.de >> 8), (uint8_t)(cpu->de >> 8));
  if (t->expect.r8_mask & TV_R8_E)
    fail |= check_u8(name, "e", (uint8_t)(t->expect.de & 0xFF), (uint8_t)(cpu->de & 0xFF));
  if (t->expect.r8_mask & TV_R8_H)
    fail |= check_u8(name, "h", (uint8_t)(t->expect.hl >> 8), (uint8_t)(cpu->hl >> 8));
  if (t->expect.r8_mask & TV_R8_L)
    fail |= check_u8(name, "l", (uint8_t)(t->expect.hl & 0xFF), (uint8_t)(cpu->hl & 0xFF));

  uint8_t got_f = (uint8_t)(cpu->af & 0xFF);
  static const struct {
    uint32_t mask;
    uint8_t bit;
    const char *name;
  } flag_map[] = {
      {TV_MASK_FLAG_S, 0x80, "flag.s"},
      {TV_MASK_FLAG_Z, 0x40, "flag.z"},
      {TV_MASK_FLAG_Y, 0x20, "flag.y"},
      {TV_MASK_FLAG_X, 0x08, "flag.x"},
      {TV_MASK_FLAG_H, 0x10, "flag.h"},
      {TV_MASK_FLAG_PV, 0x04, "flag.pv"},
      {TV_MASK_FLAG_N, 0x02, "flag.n"},
      {TV_MASK_FLAG_C, 0x01, "flag.c"},
  };
  for (size_t i = 0; i < sizeof flag_map / sizeof flag_map[0]; i++) {
    if (!(m & flag_map[i].mask)) continue;
    uint8_t exp_bit = (t->expect.flags & flag_map[i].bit) ? 1 : 0;
    uint8_t got_bit = (got_f & flag_map[i].bit) ? 1 : 0;
    if (exp_bit != got_bit) {
      fprintf(stderr, "FAIL %s: expect %s=%u got %u\n", name, flag_map[i].name, exp_bit, got_bit);
      fail = 1;
    }
  }

  if (m & TV_MASK_HALTED) {
    if (cpu->halted != 1) {
      fprintf(stderr, "FAIL %s: expect halted=1 got 0\n", name);
      fail = 1;
    }
  }

  if (m & TV_MASK_FETCH_TRAP) {
    if (!z80_fetch_trap(cpu)) {
      fprintf(stderr, "FAIL %s: expect trap=1 got 0\n", name);
      fail = 1;
    }
  }

  for (uint16_t i = 0; i < t->expect_mem_count; i++) {
    uint16_t addr = t->expect_mem[i].addr;
    uint8_t exp = t->expect_mem[i].val;
    uint8_t got = bus->mem[addr];
    if (exp != got) {
      fprintf(stderr, "FAIL %s: expect mem[0x%04X]=0x%02X got 0x%02X\n", name, addr, exp, got);
      fail = 1;
    }
  }

  return fail;
}

static int run_one_test(const tv_file_t *file, const tv_test_t *t) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);

  if (t->code_off + t->code_len > file->code_blob_size) {
    fprintf(stderr, "FAIL %s: invalid code slice\n", t->name);
    return 1;
  }

  memcpy(&bus.mem[t->start_pc], file->code_blob + t->code_off, t->code_len);

  for (uint16_t i = 0; i < t->mem_count; i++) {
    bus.mem[t->mem[i].addr] = t->mem[i].val;
  }
  for (uint16_t i = 0; i < t->io_count; i++) {
    bus.io[t->io[i].addr & 0xFF] = t->io[i].val;
  }

  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  apply_init(&cpu, t);
  z80_set_fetch_guard(&cpu, t->start_pc, t->end_pc);

  unsigned steps = 0;
  while (!cpu.halted && steps < TV_MAX_STEPS) {
    z80_step_m(&cpu);
    steps++;
  }

  if (!(t->expect_mask & TV_MASK_FETCH_TRAP) && z80_fetch_trap(&cpu)) {
    fprintf(stderr, "FAIL %s: fetch outside [0x%04X,0x%04X) at pc=0x%04X\n", t->name, t->start_pc,
            t->end_pc, z80_fetch_trap_pc(&cpu));
    return 1;
  }

  if (!(t->expect_mask & TV_MASK_FETCH_TRAP) && !(t->expect_mask & TV_MASK_HALTED) && !cpu.halted) {
    fprintf(stderr, "FAIL %s: timeout after %u steps (no HALT)\n", t->name, TV_MAX_STEPS);
    return 1;
  }

  if (!(t->expect_mask & TV_MASK_HALTED) && !(t->expect_mask & TV_MASK_FETCH_TRAP) && !cpu.halted &&
      steps >= TV_MAX_STEPS) {
    fprintf(stderr, "FAIL %s: timeout after %u steps\n", t->name, TV_MAX_STEPS);
    return 1;
  }

  return check_expect(t, &cpu, &bus);
}

int main(int argc, char **argv) {
  const char *path = (argc > 1) ? argv[1] : "tests/test_vectors.bin";
  tv_file_t file;

  if (tv_load_file(path, &file) != 0) {
    fprintf(stderr, "error: cannot load %s\n", path);
    return 1;
  }

  int passed = 0;
  int failed = 0;
  for (uint16_t i = 0; i < file.count; i++) {
    if (run_one_test(&file, &file.tests[i]) != 0) {
      failed++;
    } else {
      passed++;
    }
  }

  printf("%d passed, %d failed\n", passed, failed);
  tv_free_file(&file);
  return failed ? 1 : 0;
}
