#include <assert.h>
#include <stdio.h>
#include <string.h>

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

static void load(bus_t *b, uint16_t addr, const uint8_t *bytes, size_t n) {
  memcpy(&b->mem[addr], bytes, n);
}

static void run_ms(z80_t *cpu, unsigned n) {
  z80_run_m(cpu, n);
}

static int tests_ok = 0;
static int tests_fail = 0;

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (cond) {                                                                \
      tests_ok++;                                                              \
    } else {                                                                   \
      tests_fail++;                                                            \
      fprintf(stderr, "FAIL: %s\n", msg);                                      \
    }                                                                          \
  } while (0)

static void test_nop(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  uint8_t prog[] = {0x00};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  uint16_t pc0 = cpu.pc;
  z80_step_m(&cpu);
  CHECK(cpu.pc == pc0 + 1, "NOP advances PC by 1");
  CHECK(cpu.phase == Z80_PH_M1, "NOP returns to M1");
}

static void test_ld_r_r(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* LD B,C : 01 000 001 = 0x41 */
  uint8_t prog[] = {0x41};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.bc = 0x00AB; /* B=0, C=0xAB */
  z80_step_m(&cpu);
  CHECK(((cpu.bc >> 8) & 0xFF) == 0xAB, "LD B,C copies C to B");
  CHECK(cpu.phase == Z80_PH_M1, "LD B,C done in M1");
}

static void test_alu_add(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* ADD A,B : 10000000 = 0x80 */
  uint8_t prog[] = {0x80};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.af = 0x1000; /* A=0x10 */
  cpu.bc = 0x2000; /* B=0x20 */
  z80_step_m(&cpu);
  CHECK(((cpu.af >> 8) & 0xFF) == 0x30, "ADD A,B result");
}

static void test_ld_rp_nn(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* LD HL,0x1234 : 0x21 0x34 0x12 */
  uint8_t prog[] = {0x21, 0x34, 0x12};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  run_ms(&cpu, 3);
  CHECK(cpu.hl == 0x1234, "LD HL,nn");
  CHECK(cpu.pc == 3, "LD HL,nn PC");
  CHECK(cpu.phase == Z80_PH_M1, "LD HL,nn phase M1");
}

static void test_jp_nn(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  uint8_t prog[] = {0xC3, 0x00, 0x40}; /* JP 0x4000 */
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  run_ms(&cpu, 3);
  CHECK(cpu.pc == 0x4000, "JP nn");
}

static void test_jr_cc_taken(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* JR Z,$+2 → 0x28 0x00 (offset 0 keeps pc at 2) — use offset +2 to land at 4 */
  uint8_t prog[] = {0x28, 0x02, 0x00, 0x00, 0x00};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.af = 0x0040; /* Z set */
  run_ms(&cpu, 2);
  CHECK(cpu.pc == 4, "JR Z taken");
}

static void test_halt(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  uint8_t prog[] = {0x76};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  z80_step_m(&cpu);
  CHECK(cpu.halted, "HALT sets halted");
  uint16_t pc = cpu.pc;
  z80_step_m(&cpu);
  CHECK(cpu.pc == pc, "HALT step is no-op");
  z80_reset(&cpu);
  CHECK(!cpu.halted, "reset clears halted");
}

static void test_cb_rlc_b(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* CB 00 = RLC B */
  uint8_t prog[] = {0xCB, 0x00};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.bc = 0x8000; /* B=0x80 */
  run_ms(&cpu, 2); /* M1 CB prefix + M1_CB opcode */
  CHECK(((cpu.bc >> 8) & 0xFF) == 0x01, "RLC B");
  CHECK((cpu.af & 0x01) != 0, "RLC B sets C");
}

static void test_ed_in_c(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* ED 40 = IN B,(C) */
  uint8_t prog[] = {0xED, 0x40};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.bc = 0x0012; /* C=0x12 */
  bus.io[0x12] = 0x5A;
  run_ms(&cpu, 3); /* ED prefix M1, ED opcode M1, IO M2 */
  CHECK(((cpu.bc >> 8) & 0xFF) == 0x5A, "IN B,(C)");
}

static void test_ldir(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* ED B0 = LDIR */
  uint8_t prog[] = {0xED, 0xB0};
  load(&bus, 0, prog, sizeof prog);
  bus.mem[0x1000] = 0x11;
  bus.mem[0x1001] = 0x22;
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.hl = 0x1000;
  cpu.de = 0x2000;
  cpu.bc = 2;
  /* Enough M-cycles for 2 transfer units */
  run_ms(&cpu, 20);
  CHECK(bus.mem[0x2000] == 0x11 && bus.mem[0x2001] == 0x22, "LDIR copies");
  CHECK(cpu.bc == 0, "LDIR BC=0");
  CHECK(cpu.phase == Z80_PH_M1, "LDIR finished");
}

static void test_dd_ld_r_mem(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* DD 7E 05 = LD A,(IX+5) */
  uint8_t prog[] = {0xDD, 0x7E, 0x05};
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.ix = 0x3000;
  bus.mem[0x3005] = 0x99;
  run_ms(&cpu, 8);
  CHECK(((cpu.af >> 8) & 0xFF) == 0x99, "LD A,(IX+d)");
  CHECK(cpu.index == Z80_IDX_NONE, "index cleared after insn");
}

static void test_daa(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  uint8_t prog[] = {0x27}; /* DAA */
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.af = 0x0900; /* A=9 after adding BCD-ish */
  z80_step_m(&cpu);
  CHECK(((cpu.af >> 8) & 0xFF) == 0x09, "DAA leaves 09 alone");
}

static void test_ex_isp_hl(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  uint8_t prog[] = {0xE3}; /* EX (SP),HL */
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.sp = 0x8000;
  cpu.hl = 0x1234;
  bus.mem[0x8000] = 0x78;
  bus.mem[0x8001] = 0x56;
  run_ms(&cpu, 5);
  CHECK(cpu.hl == 0x5678, "EX (SP),HL loads stack into HL");
  CHECK(bus.mem[0x8000] == 0x34 && bus.mem[0x8001] == 0x12, "EX (SP),HL writes HL to stack");
}

static void test_ed_neg(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  uint8_t prog[] = {0xED, 0x44}; /* NEG */
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.af = 0x0100;
  run_ms(&cpu, 2);
  CHECK(((cpu.af >> 8) & 0xFF) == 0xFF, "NEG 1 -> 0xFF");
}

static void test_ed_adc_hl(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  uint8_t prog[] = {0xED, 0x5A}; /* ADC HL,DE */
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.hl = 0x1000;
  cpu.de = 0x0200;
  cpu.af = 0x0001; /* C set */
  run_ms(&cpu, 5);
  CHECK(cpu.hl == 0x1201, "ADC HL,DE with carry");
}

static void test_ed_ld_rp_inn(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  /* ED 4B lo hi = LD BC,(nn) */
  uint8_t prog[] = {0xED, 0x4B, 0x00, 0x40};
  load(&bus, 0, prog, sizeof prog);
  bus.mem[0x4000] = 0x34;
  bus.mem[0x4001] = 0x12;
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  run_ms(&cpu, 6);
  CHECK(cpu.bc == 0x1234, "LD BC,(nn)");
}

static void test_ed_cpi(void) {
  bus_t bus;
  z80_t cpu;
  memset(&bus, 0, sizeof bus);
  uint8_t prog[] = {0xED, 0xA1}; /* CPI */
  load(&bus, 0, prog, sizeof prog);
  z80_init(&cpu, (z80_bus_t){mem_read, mem_write, io_read, io_write, &bus});
  cpu.af = 0x5500;
  cpu.hl = 0x1000;
  cpu.bc = 1;
  bus.mem[0x1000] = 0x55;
  run_ms(&cpu, 6);
  CHECK(cpu.hl == 0x1001, "CPI increments HL");
  CHECK(cpu.bc == 0, "CPI decrements BC");
  CHECK((cpu.af & 0x40) != 0, "CPI sets Z on match");
}

int main(void) {
  test_nop();
  test_ld_r_r();
  test_alu_add();
  test_ld_rp_nn();
  test_jp_nn();
  test_jr_cc_taken();
  test_halt();
  test_cb_rlc_b();
  test_ed_in_c();
  test_ldir();
  test_dd_ld_r_mem();
  test_daa();
  test_ex_isp_hl();
  test_ed_neg();
  test_ed_adc_hl();
  test_ed_ld_rp_inn();
  test_ed_cpi();

  printf("%d passed, %d failed\n", tests_ok, tests_fail);
  return tests_fail ? 1 : 0;
}
