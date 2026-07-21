#!/usr/bin/env python3
"""Generate Z80 M-cycle simulator decode tables and dispatch from z80_simple*.json."""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "src" / "generated"

JSON_ROOT = REPO_ROOT / "z80_simple.json"
JSON_CB = REPO_ROOT / "z80_simple_cb.json"
JSON_ED = REPO_ROOT / "z80_simple_ed.json"

BIND_NONE = 0
BIND_LD_R_R = 1
BIND_ALU_A_R = 2
BIND_ALU_A_N = 3
BIND_RP = 4
BIND_R8_DDD = 5
BIND_R8_SSS = 6
BIND_CC = 7
BIND_CC_JR = 8
BIND_RST = 9
BIND_CB_SSS = 10
BIND_CB_BIT = 11
BIND_ED_DDD = 12
BIND_ED_SSS = 13
BIND_ED_RP = 14
BIND_IM = 15
BIND_RP_BCDE = 16

BIND_NAMES = {
    BIND_NONE: "Z80_BIND_NONE",
    BIND_LD_R_R: "Z80_BIND_LD_R_R",
    BIND_ALU_A_R: "Z80_BIND_ALU_A_R",
    BIND_ALU_A_N: "Z80_BIND_ALU_A_N",
    BIND_RP: "Z80_BIND_RP",
    BIND_R8_DDD: "Z80_BIND_R8_DDD",
    BIND_R8_SSS: "Z80_BIND_R8_SSS",
    BIND_CC: "Z80_BIND_CC",
    BIND_CC_JR: "Z80_BIND_CC_JR",
    BIND_RST: "Z80_BIND_RST",
    BIND_CB_SSS: "Z80_BIND_CB_SSS",
    BIND_CB_BIT: "Z80_BIND_CB_BIT",
    BIND_ED_DDD: "Z80_BIND_ED_DDD",
    BIND_ED_SSS: "Z80_BIND_ED_SSS",
    BIND_ED_RP: "Z80_BIND_ED_RP",
    BIND_IM: "Z80_BIND_IM",
    BIND_RP_BCDE: "Z80_BIND_RP_BCDE",
}

PREFIX_FAMILIES = frozenset(
    {"FAM_PREFIX_CB", "FAM_PREFIX_ED", "FAM_PREFIX_DD", "FAM_PREFIX_FD"}
)

FIELD_TOKENS = ("DDD", "SSS", "ALU", "bbb", "NN", "RP", "CC", "N")

FIELD_WIDTH_DEFAULT = {
    "RP": 2,
    "DDD": 3,
    "SSS": 3,
    "ALU": 3,
    "CC": 3,
    "bbb": 3,
    "N": 3,
    "NN": 2,
}

ROOT_FAMILY_MAP = {
    "NOP": "FAM_NOP",
    "LD rp,data": "FAM_LD_RP_NN",
    "LD (rp),A": "FAM_LD_IRP_A",
    "INC rp": "FAM_INC_RP",
    "INC ddd": "FAM_INC_R",
    "DEC ddd": "FAM_DEC_R",
    "LD ddd,data": "FAM_LD_R_N",
    "RLCA": "FAM_RLCA",
    "ADD HL,rp": "FAM_ADD_HL_RP",
    "LD A,(rp)": "FAM_LD_A_IRP",
    "DEC rp": "FAM_DEC_RP",
    "EX AF,AF'": "FAM_EX_AF",
    "RRCA": "FAM_RRCA",
    "DJNZ offset": "FAM_DJNZ",
    "RLA": "FAM_RLA",
    "JR offset": "FAM_JR",
    "RRA": "FAM_RRA",
    "JR cc,offset": "FAM_JR_CC",
    "LD (add),HL": "FAM_LD_INN_HL",
    "DAA": "FAM_DAA",
    "LD HL,(add)": "FAM_LD_HL_INN",
    "CPL": "FAM_CPL",
    "LD (add),A": "FAM_LD_INN_A",
    "SCF": "FAM_SCF",
    "LD A,(add)": "FAM_LD_A_INN",
    "CCF": "FAM_CCF",
    "LD ddd,sss": "FAM_LD_R_R",
    "HALT": "FAM_HALT",
    "alu A,sss": "FAM_ALU_A_R",
    "RET cc": "FAM_RET_CC",
    "POP rp": "FAM_POP_RP",
    "JP cc,add": "FAM_JP_CC",
    "JP add": "FAM_JP_NN",
    "CALL cc,add": "FAM_CALL_CC",
    "PUSH rp": "FAM_PUSH_RP",
    "alu A,data": "FAM_ALU_A_N",
    "RST n": "FAM_RST",
    "RET": "FAM_RET",
    "CB prefix": "FAM_PREFIX_CB",
    "CALL add": "FAM_CALL_NN",
    "OUT (port),A": "FAM_OUT_N_A",
    "EXX": "FAM_EXX",
    "IN A,(port)": "FAM_IN_A_N",
    "IX override": "FAM_PREFIX_DD",
    "EX (SP),HL": "FAM_EX_ISP_HL",
    "JP (HL)": "FAM_JP_HL",
    "EX DE,HL": "FAM_EX_DE_HL",
    "ED prefix": "FAM_PREFIX_ED",
    "DI": "FAM_DI",
    "LD SP,HL": "FAM_LD_SP_HL",
    "EI": "FAM_EI",
    "IY override": "FAM_PREFIX_FD",
}

CB_FAMILY_PREFIXES = (
    ("RLC ", "FAM_CB_RLC"),
    ("RRC ", "FAM_CB_RRC"),
    ("RL ", "FAM_CB_RL"),
    ("RR ", "FAM_CB_RR"),
    ("SLA ", "FAM_CB_SLA"),
    ("SRA ", "FAM_CB_SRA"),
    ("SRL ", "FAM_CB_SRL"),
    ("BIT ", "FAM_CB_BIT"),
    ("RES ", "FAM_CB_RES"),
    ("SET ", "FAM_CB_SET"),
)

ED_FAMILY_MAP = {
    "IN ddd,(C)": "FAM_ED_IN_R_C",
    "OUT (C),sss": "FAM_ED_OUT_C_R",
    "SBC HL,ss": "FAM_ED_SBC_HL",
    "LD (add),ss": "FAM_ED_LD_INN_RP",
    "NEG": "FAM_ED_NEG",
    "RETN": "FAM_ED_RETN",
    "IM n": "FAM_ED_IM",
    "LD I,A": "FAM_ED_LD_I_A",
    "ADC HL,ss": "FAM_ED_ADC_HL",
    "LD dd,(add)": "FAM_ED_LD_RP_INN",
    "RETI": "FAM_ED_RETI",
    "LD R,A": "FAM_ED_LD_R_A",
    "LD A,I": "FAM_ED_LD_A_I",
    "LD A,R": "FAM_ED_LD_A_R",
    "RRD": "FAM_ED_RRD",
    "RLD": "FAM_ED_RLD",
    "LDI": "FAM_ED_LDI",
    "LDIR": "FAM_ED_LDIR",
    "LDD": "FAM_ED_LDD",
    "LDDR": "FAM_ED_LDDR",
    "CPI": "FAM_ED_CPI",
    "CPIR": "FAM_ED_CPIR",
    "CPD": "FAM_ED_CPD",
    "CPDR": "FAM_ED_CPDR",
    "INI": "FAM_ED_INI",
    "INIR": "FAM_ED_INIR",
    "IND": "FAM_ED_IND",
    "INDR": "FAM_ED_INDR",
    "OUTI": "FAM_ED_OUTI",
    "OTIR": "FAM_ED_OTIR",
    "OUTD": "FAM_ED_OUTD",
    "OTDR": "FAM_ED_OTDR",
}

FAMILY_BIND = {
    "FAM_NOP": BIND_NONE,
    "FAM_LD_RP_NN": BIND_RP,
    "FAM_LD_IRP_A": BIND_RP_BCDE,
    "FAM_INC_RP": BIND_RP,
    "FAM_INC_R": BIND_R8_DDD,
    "FAM_DEC_R": BIND_R8_DDD,
    "FAM_LD_R_N": BIND_R8_DDD,
    "FAM_RLCA": BIND_NONE,
    "FAM_ADD_HL_RP": BIND_RP,
    "FAM_LD_A_IRP": BIND_RP_BCDE,
    "FAM_DEC_RP": BIND_RP,
    "FAM_EX_AF": BIND_NONE,
    "FAM_RRCA": BIND_NONE,
    "FAM_DJNZ": BIND_NONE,
    "FAM_RLA": BIND_NONE,
    "FAM_JR": BIND_NONE,
    "FAM_RRA": BIND_NONE,
    "FAM_JR_CC": BIND_CC_JR,
    "FAM_LD_INN_HL": BIND_NONE,
    "FAM_DAA": BIND_NONE,
    "FAM_LD_HL_INN": BIND_NONE,
    "FAM_CPL": BIND_NONE,
    "FAM_LD_INN_A": BIND_NONE,
    "FAM_SCF": BIND_NONE,
    "FAM_LD_A_INN": BIND_NONE,
    "FAM_CCF": BIND_NONE,
    "FAM_LD_R_R": BIND_LD_R_R,
    "FAM_HALT": BIND_NONE,
    "FAM_ALU_A_R": BIND_ALU_A_R,
    "FAM_RET_CC": BIND_CC,
    "FAM_POP_RP": BIND_RP,
    "FAM_JP_CC": BIND_CC,
    "FAM_JP_NN": BIND_NONE,
    "FAM_CALL_CC": BIND_CC,
    "FAM_PUSH_RP": BIND_RP,
    "FAM_ALU_A_N": BIND_ALU_A_N,
    "FAM_RST": BIND_RST,
    "FAM_RET": BIND_NONE,
    "FAM_PREFIX_CB": BIND_NONE,
    "FAM_CALL_NN": BIND_NONE,
    "FAM_OUT_N_A": BIND_NONE,
    "FAM_EXX": BIND_NONE,
    "FAM_IN_A_N": BIND_NONE,
    "FAM_PREFIX_DD": BIND_NONE,
    "FAM_EX_ISP_HL": BIND_NONE,
    "FAM_JP_HL": BIND_NONE,
    "FAM_EX_DE_HL": BIND_NONE,
    "FAM_PREFIX_ED": BIND_NONE,
    "FAM_DI": BIND_NONE,
    "FAM_LD_SP_HL": BIND_NONE,
    "FAM_EI": BIND_NONE,
    "FAM_PREFIX_FD": BIND_NONE,
    "FAM_UNDOC": BIND_NONE,
    "FAM_CB_RLC": BIND_CB_SSS,
    "FAM_CB_RRC": BIND_CB_SSS,
    "FAM_CB_RL": BIND_CB_SSS,
    "FAM_CB_RR": BIND_CB_SSS,
    "FAM_CB_SLA": BIND_CB_SSS,
    "FAM_CB_SRA": BIND_CB_SSS,
    "FAM_CB_SRL": BIND_CB_SSS,
    "FAM_CB_BIT": BIND_CB_BIT,
    "FAM_CB_RES": BIND_CB_BIT,
    "FAM_CB_SET": BIND_CB_BIT,
    "FAM_CB_UNDOC": BIND_NONE,
    "FAM_ED_IN_R_C": BIND_ED_DDD,
    "FAM_ED_OUT_C_R": BIND_ED_SSS,
    "FAM_ED_SBC_HL": BIND_ED_RP,
    "FAM_ED_LD_INN_RP": BIND_ED_RP,
    "FAM_ED_NEG": BIND_NONE,
    "FAM_ED_RETN": BIND_NONE,
    "FAM_ED_IM": BIND_IM,
    "FAM_ED_LD_I_A": BIND_NONE,
    "FAM_ED_ADC_HL": BIND_ED_RP,
    "FAM_ED_LD_RP_INN": BIND_ED_RP,
    "FAM_ED_RETI": BIND_NONE,
    "FAM_ED_LD_R_A": BIND_NONE,
    "FAM_ED_LD_A_I": BIND_NONE,
    "FAM_ED_LD_A_R": BIND_NONE,
    "FAM_ED_RRD": BIND_NONE,
    "FAM_ED_RLD": BIND_NONE,
    "FAM_ED_LDI": BIND_NONE,
    "FAM_ED_LDIR": BIND_NONE,
    "FAM_ED_LDD": BIND_NONE,
    "FAM_ED_LDDR": BIND_NONE,
    "FAM_ED_CPI": BIND_NONE,
    "FAM_ED_CPIR": BIND_NONE,
    "FAM_ED_CPD": BIND_NONE,
    "FAM_ED_CPDR": BIND_NONE,
    "FAM_ED_INI": BIND_NONE,
    "FAM_ED_INIR": BIND_NONE,
    "FAM_ED_IND": BIND_NONE,
    "FAM_ED_INDR": BIND_NONE,
    "FAM_ED_OUTI": BIND_NONE,
    "FAM_ED_OTIR": BIND_NONE,
    "FAM_ED_OUTD": BIND_NONE,
    "FAM_ED_OTDR": BIND_NONE,
    "FAM_ED_NOP": BIND_NONE,
}

FAMILY_M_EXTRA = {
    "FAM_NOP": 0,
    "FAM_HALT": 0,
    "FAM_DI": 0,
    "FAM_EI": 0,
    "FAM_EX_AF": 0,
    "FAM_EXX": 0,
    "FAM_EX_DE_HL": 0,
    "FAM_EX_ISP_HL": 4,
    "FAM_SCF": 0,
    "FAM_CCF": 0,
    "FAM_CPL": 0,
    "FAM_DAA": 0,
    "FAM_RLCA": 0,
    "FAM_RRCA": 0,
    "FAM_RLA": 0,
    "FAM_RRA": 0,
    "FAM_LD_R_R": 0,
    "FAM_ALU_A_R": 0,
    "FAM_INC_R": 0,
    "FAM_DEC_R": 0,
    "FAM_JP_HL": 0,
    "FAM_LD_SP_HL": 0,
    "FAM_PREFIX_CB": 0,
    "FAM_PREFIX_ED": 0,
    "FAM_PREFIX_DD": 0,
    "FAM_PREFIX_FD": 0,
    "FAM_LD_R_N": 1,
    "FAM_JR": 1,
    "FAM_JR_CC": 1,
    "FAM_DJNZ": 1,
    "FAM_LD_RP_NN": 2,
    "FAM_JP_NN": 2,
    "FAM_ALU_A_N": 2,
    "FAM_IN_A_N": 2,
    "FAM_OUT_N_A": 2,
    "FAM_LD_INN_HL": 2,
    "FAM_LD_HL_INN": 2,
    "FAM_LD_INN_A": 2,
    "FAM_LD_A_INN": 2,
    "FAM_CALL_NN": 3,
    "FAM_CALL_CC": 3,
    "FAM_PUSH_RP": 3,
    "FAM_POP_RP": 2,
    "FAM_RET": 2,
    "FAM_RET_CC": 2,
    "FAM_RST": 2,
    "FAM_INC_RP": 0,
    "FAM_DEC_RP": 0,
    "FAM_ADD_HL_RP": 2,
    "FAM_LD_IRP_A": 1,
    "FAM_LD_A_IRP": 1,
    "FAM_UNDOC": 0,
    "FAM_CB_RLC": 0,
    "FAM_CB_RRC": 0,
    "FAM_CB_RL": 0,
    "FAM_CB_RR": 0,
    "FAM_CB_SLA": 0,
    "FAM_CB_SRA": 0,
    "FAM_CB_SRL": 0,
    "FAM_CB_BIT": 0,
    "FAM_CB_RES": 0,
    "FAM_CB_SET": 0,
    "FAM_CB_UNDOC": 0,
    "FAM_ED_IN_R_C": 1,
    "FAM_ED_OUT_C_R": 1,
    "FAM_ED_SBC_HL": 2,
    "FAM_ED_ADC_HL": 2,
    "FAM_ED_LD_INN_RP": 2,
    "FAM_ED_LD_RP_INN": 2,
    "FAM_ED_NEG": 0,
    "FAM_ED_RETN": 2,
    "FAM_ED_RETI": 2,
    "FAM_ED_IM": 0,
    "FAM_ED_LD_I_A": 0,
    "FAM_ED_LD_R_A": 0,
    "FAM_ED_LD_A_I": 0,
    "FAM_ED_LD_A_R": 0,
    "FAM_ED_RRD": 2,
    "FAM_ED_RLD": 2,
    "FAM_ED_LDI": 4,
    "FAM_ED_LDIR": 4,
    "FAM_ED_LDD": 4,
    "FAM_ED_LDDR": 4,
    "FAM_ED_CPI": 4,
    "FAM_ED_CPIR": 4,
    "FAM_ED_CPD": 4,
    "FAM_ED_CPDR": 4,
    "FAM_ED_INI": 4,
    "FAM_ED_INIR": 4,
    "FAM_ED_IND": 4,
    "FAM_ED_INDR": 4,
    "FAM_ED_OUTI": 4,
    "FAM_ED_OTIR": 4,
    "FAM_ED_OUTD": 4,
    "FAM_ED_OTDR": 4,
    "FAM_ED_NOP": 0,
}


@dataclass(frozen=True)
class DecodeEntry:
    family: str
    bind: int
    m_extra: int
    mnemonic: str = ""
    description: str = ""


@dataclass(frozen=True)
class PatternSpec:
    bits: tuple[str | int, ...]


def load_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as fh:
        return json.load(fh)


def family_for_root(mnemonic: str) -> str:
    family = ROOT_FAMILY_MAP.get(mnemonic)
    if family is None:
        raise KeyError(f"Unmapped root mnemonic: {mnemonic!r}")
    return family


def family_for_cb(mnemonic: str) -> str:
    for prefix, family in CB_FAMILY_PREFIXES:
        if mnemonic.startswith(prefix):
            return family
    raise KeyError(f"Unmapped CB mnemonic: {mnemonic!r}")


def family_for_ed(mnemonic: str) -> str:
    family = ED_FAMILY_MAP.get(mnemonic)
    if family is None:
        raise KeyError(f"Unmapped ED mnemonic: {mnemonic!r}")
    return family


def cc_width(mnemonic: str) -> int:
    return 2 if mnemonic.startswith("JR cc") else 3


def pattern_candidates(pattern: str) -> list[str]:
    """Yield normalization attempts for JSON patterns that encode 8 opcode bits.

    Wikipedia-style dumps often write ``000RP001`` (3+2+3) instead of the real
    Z80 layout ``00RP0001`` (2+2+4). Prefer candidates that match hardware encoding.
    """
    preferred: list[str] = []
    fallback: list[str] = [pattern]
    # 000RP001 → 00RP0001 (and same for 000RP010 / 000RP011)
    if pattern.startswith("000RP") and len(pattern) == 8:
        preferred.append("00RP0" + pattern[5:])
    if len(pattern) == 9:
        if pattern[2] == "0":
            preferred.append(pattern[:2] + pattern[3:])
        if pattern.startswith("1010"):
            preferred.append(pattern[:3] + pattern[4:])
        if all(ch in "01" for ch in pattern):
            preferred.append(pattern[:8])
    seen: set[str] = set()
    ordered: list[str] = []
    for cand in preferred + fallback:
        if cand not in seen:
            seen.add(cand)
            ordered.append(cand)
    return ordered
def tokenize_pattern(pattern: str, mnemonic: str) -> tuple[str | int, ...]:
    bits: list[str | int] = []
    i = 0
    while i < len(pattern):
        ch = pattern[i]
        if ch in "01":
            bits.append(int(ch))
            i += 1
            continue
        matched = None
        for token in sorted(FIELD_TOKENS, key=len, reverse=True):
            if pattern[i : i + len(token)] == token:
                matched = token
                break
        if matched is None:
            raise ValueError(f"Unknown token in pattern {pattern!r} at {i}")
        width = cc_width(mnemonic) if matched == "CC" else FIELD_WIDTH_DEFAULT[matched]
        bits.extend([matched] * width)
        i += len(matched)
    return tuple(bits)


def parse_pattern(pattern: str, mnemonic: str) -> PatternSpec:
    last_error: ValueError | None = None
    for candidate in pattern_candidates(pattern):
        try:
            bits = tokenize_pattern(candidate, mnemonic)
        except ValueError as exc:
            last_error = exc
            continue
        if len(bits) == 8:
            return PatternSpec(bits)
        last_error = ValueError(
            f"Pattern {candidate!r} for {mnemonic!r} is {len(bits)} bits, expected 8"
        )
    if last_error is not None:
        raise last_error
    raise ValueError(f"Pattern {pattern!r} for {mnemonic!r} could not be parsed")


def bits_to_opcode(bits: tuple[str | int, ...], values: dict[str, int]) -> int:
    field_counters: dict[str, int] = {}
    out = 0
    for bit in bits:
        out <<= 1
        if isinstance(bit, int):
            out |= bit
            continue
        width = sum(1 for b in bits if b == bit)
        idx = field_counters.get(bit, 0)
        field_counters[bit] = idx + 1
        shift = width - 1 - idx
        out |= (values[bit] >> shift) & 1
    return out


def expand_pattern(pattern: str, mnemonic: str) -> Iterable[int]:
    spec = parse_pattern(pattern, mnemonic)
    fields: list[str] = []
    for bit in spec.bits:
        if isinstance(bit, str) and bit not in fields:
            fields.append(bit)
    if not fields:
        yield bits_to_opcode(spec.bits, {})
        return
    widths = {field: sum(1 for b in spec.bits if b == field) for field in fields}

    def rec(idx: int, values: dict[str, int]) -> Iterable[int]:
        if idx == len(fields):
            yield bits_to_opcode(spec.bits, values)
            return
        field = fields[idx]
        for v in range(1 << widths[field]):
            values[field] = v
            yield from rec(idx + 1, values)

    yield from rec(0, {})


def make_entry(family: str, mnemonic: str, description: str) -> DecodeEntry:
    bind = FAMILY_BIND[family]
    m_extra = FAMILY_M_EXTRA.get(family, 0)
    return DecodeEntry(family=family, bind=bind, m_extra=m_extra, mnemonic=mnemonic, description=description)


def build_table(instructions: list[dict], default_family: str, family_fn) -> list[DecodeEntry]:
    table = [make_entry(default_family, "", "unmapped") for _ in range(256)]
    for inst in instructions:
        mnemonic = inst["mnemonic"]
        family = family_fn(mnemonic)
        entry = make_entry(family, mnemonic, inst.get("description", ""))
        for opcode in expand_pattern(inst["opcode"], mnemonic):
            table[opcode] = entry
    return table


def fam_to_snake(family: str) -> str:
    return "z80_m_" + family[4:].lower()


def collect_families(*tables: list[DecodeEntry]) -> list[str]:
    seen: set[str] = set()
    ordered: list[str] = []

    def add(name: str) -> None:
        if name not in seen:
            seen.add(name)
            ordered.append(name)

    for fam in sorted(FAMILY_BIND):
        add(fam)
    for table in tables:
        for ent in table:
            add(ent.family)
    return ordered


def count_filled(table: list[DecodeEntry], default_family: str) -> int:
    return sum(1 for ent in table if ent.family != default_family)


def emit_families_h(families: list[str]) -> str:
    lines = [
        "/* Auto-generated by tools/gen_z80_core.py — do not edit. */",
        "#ifndef Z80_FAMILIES_H",
        "#define Z80_FAMILIES_H",
        "",
        "typedef enum {",
    ]
    for fam in families:
        if fam != "FAM_COUNT":
            lines.append(f"  {fam},")
    lines.append("  FAM_COUNT")
    lines.append("} z80_family_t;")
    lines.append("")
    lines.append("#endif /* Z80_FAMILIES_H */")
    lines.append("")
    return "\n".join(lines)


def emit_decode_h() -> str:
    return "\n".join([
        "/* Auto-generated by tools/gen_z80_core.py — do not edit. */",
        "#ifndef Z80_DECODE_H",
        "#define Z80_DECODE_H",
        "",
        "#include <stdint.h>",
        "",
        '#include "z80_families.h"',
        "",
        "#define Z80_BIND_NONE     0",
        "#define Z80_BIND_LD_R_R   1",
        "#define Z80_BIND_ALU_A_R  2",
        "#define Z80_BIND_ALU_A_N  3",
        "#define Z80_BIND_RP       4",
        "#define Z80_BIND_R8_DDD   5",
        "#define Z80_BIND_R8_SSS   6",
        "#define Z80_BIND_CC       7",
        "#define Z80_BIND_CC_JR    8",
        "#define Z80_BIND_RST      9",
        "#define Z80_BIND_CB_SSS   10",
        "#define Z80_BIND_CB_BIT   11",
        "#define Z80_BIND_ED_DDD   12",
        "#define Z80_BIND_ED_SSS   13",
        "#define Z80_BIND_ED_RP    14",
        "#define Z80_BIND_IM       15",
        "#define Z80_BIND_RP_BCDE  16",
        "",
        "typedef struct {",
        "  uint16_t family; /* z80_family_t */",
        "  uint8_t bind;",
        "  uint8_t m_extra;",
        "} z80_decode_ent;",
        "",
        "extern const z80_decode_ent z80_decode_root[256];",
        "extern const z80_decode_ent z80_decode_cb[256];",
        "extern const z80_decode_ent z80_decode_ed[256];",
        "",
        "#endif /* Z80_DECODE_H */",
        "",
    ])


def emit_decode_c(name: str, table: list[DecodeEntry]) -> str:
    lines = [
        "/* Auto-generated by tools/gen_z80_core.py — do not edit. */",
        '#include "z80_decode.h"',
        "",
        f"const z80_decode_ent {name}[256] = {{",
    ]
    for opcode, ent in enumerate(table):
        bind_name = BIND_NAMES[ent.bind]
        lines.append(
            f"  [0x{opcode:02X}] = {{ .family = {ent.family}, "
            f".bind = {bind_name}, .m_extra = {ent.m_extra} }},"
        )
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def gather_family_notes(*tables: list[DecodeEntry]) -> dict[str, list[str]]:
    notes: dict[str, list[str]] = {}
    for table in tables:
        for ent in table:
            if ent.family in PREFIX_FAMILIES or not ent.mnemonic:
                continue
            note = ent.mnemonic
            if ent.description:
                note = f"{ent.mnemonic}: {ent.description}"
            notes.setdefault(ent.family, []).append(note)
    return notes


def emit_handlers_h(families: list[str], family_notes: dict[str, list[str]]) -> str:
    lines = [
        "/* Auto-generated by tools/gen_z80_core.py — do not edit. */",
        "#ifndef Z80_HANDLERS_H",
        "#define Z80_HANDLERS_H",
        "",
        "typedef struct z80 z80_t;",
        "",
    ]
    skip = PREFIX_FAMILIES | {"FAM_COUNT"}
    for fam in families:
        if fam in skip:
            continue
        handler = fam_to_snake(fam)
        for note in sorted(set(family_notes.get(fam, []))):
            lines.append(f"/* {note} */")
        lines.append(f"void {handler}(z80_t *cpu);")
        lines.append("")
    lines.append("#endif /* Z80_HANDLERS_H */")
    lines.append("")
    return "\n".join(lines)


def emit_dispatch_h() -> str:
    return "\n".join([
        "/* Auto-generated by tools/gen_z80_core.py — do not edit. */",
        "#ifndef Z80_DISPATCH_H",
        "#define Z80_DISPATCH_H",
        "",
        "typedef struct z80 z80_t;",
        "",
        "void z80_dispatch_exec(z80_t *cpu);",
        "",
        "#endif /* Z80_DISPATCH_H */",
        "",
    ])


def emit_dispatch_c(exec_families: list[str]) -> str:
    lines = [
        "/* Auto-generated by tools/gen_z80_core.py — do not edit. */",
        "#include <assert.h>",
        "",
        '#include "z80.h"',
        '#include "z80_dispatch.h"',
        '#include "z80_families.h"',
        '#include "z80_handlers.h"',
        "",
        "void z80_dispatch_exec(z80_t *cpu) {",
        "  switch (cpu->family) {",
    ]
    for fam in exec_families:
        lines.append(f"  case {fam}:")
        lines.append(f"    {fam_to_snake(fam)}(cpu);")
        lines.append("    break;")
    lines.extend([
        "  default:",
        '    assert(0 && "unhandled z80 family");',
        "    break;",
        "  }",
        "}",
        "",
    ])
    return "\n".join(lines)


def print_stub_suggestions(exec_families: list[str]) -> None:
    print("\nSuggested handler stubs (copy into hand-written .c files):")
    for fam in exec_families:
        handler = fam_to_snake(fam)
        print(f'void {handler}(z80_t *cpu) {{ assert(0 && "{fam} not implemented"); }}')


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def verify_halt_collision(table: list[DecodeEntry]) -> None:
    if table[0x76].family != "FAM_HALT":
        print(
            f"warning: opcode 0x76 mapped to {table[0x76].family}, expected FAM_HALT",
            file=sys.stderr,
        )


def main() -> int:
    root_data = load_json(JSON_ROOT)
    cb_data = load_json(JSON_CB)
    ed_data = load_json(JSON_ED)

    root_table = build_table(root_data["instructions"], "FAM_UNDOC", family_for_root)
    cb_table = build_table(cb_data["instructions"], "FAM_CB_UNDOC", family_for_cb)
    ed_table = build_table(ed_data["instructions"], "FAM_ED_NOP", family_for_ed)
    verify_halt_collision(root_table)

    families = collect_families(root_table, cb_table, ed_table)
    if "FAM_COUNT" in families:
        families.remove("FAM_COUNT")
    families.append("FAM_COUNT")

    family_notes = gather_family_notes(root_table, cb_table, ed_table)
    exec_families = [fam for fam in families if fam not in PREFIX_FAMILIES and fam != "FAM_COUNT"]

    write_text(OUT_DIR / "z80_families.h", emit_families_h(families))
    write_text(OUT_DIR / "z80_decode.h", emit_decode_h())
    write_text(OUT_DIR / "z80_decode_root.c", emit_decode_c("z80_decode_root", root_table))
    write_text(OUT_DIR / "z80_decode_cb.c", emit_decode_c("z80_decode_cb", cb_table))
    write_text(OUT_DIR / "z80_decode_ed.c", emit_decode_c("z80_decode_ed", ed_table))
    write_text(OUT_DIR / "z80_handlers.h", emit_handlers_h(exec_families, family_notes))
    write_text(OUT_DIR / "z80_dispatch.h", emit_dispatch_h())
    write_text(OUT_DIR / "z80_dispatch.c", emit_dispatch_c(exec_families))

    handler_count = len(exec_families)
    print(f"Generated files under {OUT_DIR}")
    print(f"  families (enum entries): {len(families) - 1}")
    print(f"  handler declarations:    {handler_count}")
    print(f"  filled root slots:       {count_filled(root_table, 'FAM_UNDOC')}/256")
    print(f"  filled cb slots:         {count_filled(cb_table, 'FAM_CB_UNDOC')}/256")
    print(f"  filled ed slots:         {count_filled(ed_table, 'FAM_ED_NOP')}/256")
    print_stub_suggestions(exec_families)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
