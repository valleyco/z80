#!/usr/bin/env python3
"""Generate tests/test_vectors.bin from asmx .asm + .lst + .bin."""

from __future__ import annotations

import argparse
import re
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

MEM_PAIR_RE = re.compile(r"(\$?[0-9a-f]+h?)\s*=\s*(\$?[0-9a-f]+h?)", re.I)

# Mask bits — must match tests/test_vectors.h
MASK = {
    "af": 1 << 0,
    "bc": 1 << 1,
    "de": 1 << 2,
    "hl": 1 << 3,
    "ix": 1 << 4,
    "iy": 1 << 5,
    "sp": 1 << 6,
    "pc": 1 << 7,
    "af_": 1 << 8,
    "bc_": 1 << 9,
    "de_": 1 << 10,
    "hl_": 1 << 11,
    "i": 1 << 12,
    "r": 1 << 13,
    "im": 1 << 14,
    "iff1": 1 << 15,
    "iff2": 1 << 16,
    "halted": 1 << 17,
    "trap": 1 << 18,
    "a": 1 << 19,
    "f": 1 << 20,
    "flag.s": 1 << 24,
    "flag.z": 1 << 25,
    "flag.y": 1 << 26,
    "flag.x": 1 << 27,
    "flag.h": 1 << 28,
    "flag.pv": 1 << 29,
    "flag.n": 1 << 30,
    "flag.c": 1 << 31,
}

FLAG_BITS = {
    "flag.s": 0x80,
    "flag.z": 0x40,
    "flag.y": 0x20,
    "flag.x": 0x08,
    "flag.h": 0x10,
    "flag.pv": 0x04,
    "flag.n": 0x02,
    "flag.c": 0x01,
}


@dataclass
class CpuState:
    af: int = 0
    bc: int = 0
    de: int = 0
    hl: int = 0
    ix: int = 0
    iy: int = 0
    sp: int = 0
    pc: int = 0
    af_: int = 0
    bc_: int = 0
    de_: int = 0
    hl_: int = 0
    i: int = 0
    r: int = 0
    im: int = 0
    iff1: int = 0
    iff2: int = 0
    flags: int = 0
    r8_mask: int = 0


# Half-register bits in CpuState.r8_mask / tv_cpu_state_t.r8_mask
R8_MASK = {
    "b": 1 << 0,
    "c": 1 << 1,
    "d": 1 << 2,
    "e": 1 << 3,
    "h": 1 << 4,
    "l": 1 << 5,
}


@dataclass
class TestCase:
    name: str
    label: str = ""
    start_pc: int = 0
    end_pc: int = 0
    init_mask: int = 0
    expect_mask: int = 0
    init: CpuState = field(default_factory=CpuState)
    expect: CpuState = field(default_factory=CpuState)
    mem: list[tuple[int, int]] = field(default_factory=list)
    io: list[tuple[int, int]] = field(default_factory=list)
    expect_mem: list[tuple[int, int]] = field(default_factory=list)


def parse_int(s: str) -> int:
    s = s.strip().lower()
    if s.endswith("h"):
        return int(s[:-1], 16)
    if s.startswith("0x"):
        return int(s, 16)
    if s.startswith("$"):
        return int(s[1:], 16)
    if s in ("0", "1"):
        return int(s)
    return int(s, 0)


def parse_pairs(text: str) -> list[tuple[str, str]]:
    pairs: list[tuple[str, str]] = []
    for part in re.split(r"\s+", text.strip()):
        if not part or "=" not in part:
            continue
        k, v = part.split("=", 1)
        pairs.append((k.strip().lower(), v.strip()))
    return pairs


def apply_field(state: CpuState, mask: int, key: str, val: int) -> int:
    key = key.lower()
    if key == "a":
        state.af = (state.af & 0x00FF) | ((val & 0xFF) << 8)
        return mask | MASK["a"]
    if key == "f":
        state.af = (state.af & 0xFF00) | (val & 0xFF)
        return mask | MASK["f"]
    if key in R8_MASK:
        bit = R8_MASK[key]
        state.r8_mask |= bit
        if key == "b":
            state.bc = (state.bc & 0x00FF) | ((val & 0xFF) << 8)
        elif key == "c":
            state.bc = (state.bc & 0xFF00) | (val & 0xFF)
        elif key == "d":
            state.de = (state.de & 0x00FF) | ((val & 0xFF) << 8)
        elif key == "e":
            state.de = (state.de & 0xFF00) | (val & 0xFF)
        elif key == "h":
            state.hl = (state.hl & 0x00FF) | ((val & 0xFF) << 8)
        elif key == "l":
            state.hl = (state.hl & 0xFF00) | (val & 0xFF)
        return mask  # half-regs use r8_mask in the state blob, not main mask
    if key.startswith("flag."):
        if val:
            state.flags |= FLAG_BITS[key]
        else:
            state.flags &= ~FLAG_BITS[key]
        return mask | MASK[key]
    if key == "halted":
        return mask | MASK["halted"]
    if key == "trap":
        return mask | MASK["trap"]
    reg_map = {
        "af": "af",
        "bc": "bc",
        "de": "de",
        "hl": "hl",
        "ix": "ix",
        "iy": "iy",
        "sp": "sp",
        "pc": "pc",
        "af_": "af_",
        "bc_": "bc_",
        "de_": "de_",
        "hl_": "hl_",
        "i": "i",
        "r": "r",
        "im": "im",
        "iff1": "iff1",
        "iff2": "iff2",
    }
    if key not in reg_map:
        raise ValueError(f"unknown field {key!r}")
    attr = reg_map[key]
    if attr in ("i", "r", "im", "iff1", "iff2"):
        setattr(state, attr, val & 0xFF)
    else:
        setattr(state, attr, val & 0xFFFF)
    return mask | MASK[key]


def parse_asm(path: Path) -> list[TestCase]:
    tests: list[TestCase] = []
    current: TestCase | None = None

    for raw in path.read_text().splitlines():
        line = raw.split(";", 1)
        code_part = line[0].strip()
        comment = line[1].strip() if len(line) > 1 else ""

        if not comment.startswith("@"):
            if code_part and not code_part.startswith(".") and code_part.endswith(":"):
                lbl = code_part[:-1].strip()
                if current is not None and not current.label:
                    current.label = lbl
            continue

        body = comment[1:].strip()
        if body.startswith("test "):
            name = body[5:].strip()
            if not name:
                raise ValueError("empty @test name")
            current = TestCase(name=name, label="")
            tests.append(current)
            continue

        if current is None:
            raise ValueError(f"directive before @test: {body}")

        if body.startswith("init "):
            for k, v in parse_pairs(body[5:]):
                current.init_mask = apply_field(current.init, current.init_mask, k, parse_int(v))
        elif body.startswith("expect "):
            for k, v in parse_pairs(body[7:]):
                current.expect_mask = apply_field(current.expect, current.expect_mask, k, parse_int(v))
        elif body.startswith("mem "):
            rest = body[4:].strip()
            pairs = MEM_PAIR_RE.findall(rest)
            if not pairs:
                raise ValueError(f"bad @mem line: {body}")
            for addr_s, val_s in pairs:
                current.mem.append((parse_int(addr_s), parse_int(val_s)))
        elif body.startswith("io "):
            m = MEM_PAIR_RE.match(body[3:].strip())
            if m:
                current.io.append((parse_int(m.group(1)), parse_int(m.group(2))))
        elif body.startswith("expect_mem "):
            rest = body[11:].strip()
            pairs = MEM_PAIR_RE.findall(rest)
            if not pairs:
                raise ValueError(f"bad @expect_mem line: {body}")
            for addr_s, val_s in pairs:
                current.expect_mem.append((parse_int(addr_s), parse_int(val_s)))

    return tests


def parse_lst(path: Path) -> dict[str, int]:
    labels: dict[str, int] = {}
    for raw in path.read_text().splitlines():
        m = re.match(r"^([0-9A-Fa-f]{4})\s+(\w+):", raw)
        if m:
            labels[m.group(2).lower()] = int(m.group(1), 16)
            continue
        m = re.match(r"^([A-Z0-9_]+)\s+([0-9A-Fa-f]{4})\s*$", raw.strip())
        if m:
            labels[m.group(1).lower()] = int(m.group(2), 16)
    return labels


def find_end_pc(lines: list[str], start: int, next_start: int) -> int:
    """Return exclusive end_pc: byte after halt, or next test start if no halt."""
    for raw in lines:
        m = re.match(r"^([0-9A-Fa-f]{4})\s+([0-9A-Fa-f]{2})\s+.*\b(halt|HALT)\b", raw)
        if not m:
            continue
        addr = int(m.group(1), 16)
        if start <= addr < next_start:
            return addr + 1
    return next_start


def pack_state(st: CpuState) -> bytes:
    return struct.pack(
        "<8H4H5BBB5x",
        st.af,
        st.bc,
        st.de,
        st.hl,
        st.ix,
        st.iy,
        st.sp,
        st.pc,
        st.af_,
        st.bc_,
        st.de_,
        st.hl_,
        st.i,
        st.r,
        st.im,
        st.iff1,
        st.iff2,
        st.flags,
        st.r8_mask & 0xFF,
    )


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate Z80 test vectors binary")
    ap.add_argument("asm", type=Path, help="Source .asm file")
    ap.add_argument("-o", "--output", type=Path, default=Path("tests/test_vectors.bin"))
    args = ap.parse_args()

    asm_path = args.asm
    lst_path = asm_path.with_suffix(".lst")
    bin_path = asm_path.with_suffix(".bin")

    if not lst_path.is_file():
        print(f"error: missing listing {lst_path}", file=sys.stderr)
        return 1
    if not bin_path.is_file():
        print(f"error: missing binary {bin_path}", file=sys.stderr)
        return 1

    tests = parse_asm(asm_path)
    if not tests:
        print("error: no tests found", file=sys.stderr)
        return 1

    labels = parse_lst(lst_path)
    lst_lines = lst_path.read_text().splitlines()
    bin_data = bin_path.read_bytes()

    names_seen: set[str] = set()
    code_blob = bytearray()
    records: list[bytes] = []

    # Resolve label addresses first
    for tc in tests:
        if tc.name in names_seen:
            print(f"error: duplicate test name {tc.name!r}", file=sys.stderr)
            return 1
        names_seen.add(tc.name)
        lbl = (tc.label or tc.name).lower()
        if lbl not in labels:
            print(f"error: label {lbl!r} not found in {lst_path}", file=sys.stderr)
            return 1
        tc.start_pc = labels[lbl]

    for i, tc in enumerate(tests):
        next_start = tests[i + 1].start_pc if i + 1 < len(tests) else 0x10000
        tc.end_pc = find_end_pc(lst_lines, tc.start_pc, next_start)
        code_len = tc.end_pc - tc.start_pc
        if code_len <= 0:
            print(f"error: test {tc.name}: invalid code length", file=sys.stderr)
            return 1

        if tc.end_pc > len(bin_data):
            print(f"error: test {tc.name}: end_pc past bin size", file=sys.stderr)
            return 1
        code_slice = bin_data[tc.start_pc : tc.end_pc]
        code_off = len(code_blob)
        code_blob.extend(code_slice)

        if not (tc.init_mask & MASK["pc"]):
            tc.init.pc = tc.start_pc
            tc.init_mask |= MASK["pc"]

        name_b = tc.name.encode("utf-8")
        rec = struct.pack("<H", len(name_b))
        rec += name_b
        rec += struct.pack("<HHIIII", tc.start_pc, tc.end_pc, code_off, code_len, tc.init_mask, tc.expect_mask)
        rec += pack_state(tc.init)
        rec += pack_state(tc.expect)
        rec += struct.pack("<HH", len(tc.mem), len(tc.io))
        for addr, val in tc.mem:
            rec += struct.pack("<HB", addr & 0xFFFF, val & 0xFF)
        for port, val in tc.io:
            rec += struct.pack("<HB", port & 0xFFFF, val & 0xFF)
        rec += struct.pack("<H", len(tc.expect_mem))
        for addr, val in tc.expect_mem:
            rec += struct.pack("<HB", addr & 0xFFFF, val & 0xFF)
        records.append(rec)

    out = bytearray()
    out += b"Z80T"
    out += struct.pack("<BHI", 1, len(tests), len(code_blob))
    for rec in records:
        out += rec
    out += code_blob

    args.output.write_bytes(out)
    print(f"Wrote {args.output} ({len(tests)} tests, {len(code_blob)} code bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
