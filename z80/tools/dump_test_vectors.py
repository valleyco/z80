#!/usr/bin/env python3
"""Dump tests/test_vectors.bin in human-readable form."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

MASK_NAMES = [
    (1 << 0, "af"),
    (1 << 1, "bc"),
    (1 << 2, "de"),
    (1 << 3, "hl"),
    (1 << 4, "ix"),
    (1 << 5, "iy"),
    (1 << 6, "sp"),
    (1 << 7, "pc"),
    (1 << 8, "af_"),
    (1 << 9, "bc_"),
    (1 << 10, "de_"),
    (1 << 11, "hl_"),
    (1 << 12, "i"),
    (1 << 13, "r"),
    (1 << 14, "im"),
    (1 << 15, "iff1"),
    (1 << 16, "iff2"),
    (1 << 17, "halted"),
    (1 << 18, "trap"),
    (1 << 19, "a"),
    (1 << 20, "f"),
    (1 << 24, "flag.s"),
    (1 << 25, "flag.z"),
    (1 << 26, "flag.y"),
    (1 << 27, "flag.x"),
    (1 << 28, "flag.h"),
    (1 << 29, "flag.pv"),
    (1 << 30, "flag.n"),
    (1 << 31, "flag.c"),
]

STATE_FMT = "<8H4H5BBB5x"


def unpack_state(data: bytes) -> dict:
    vals = struct.unpack(STATE_FMT, data)
    keys = [
        "af",
        "bc",
        "de",
        "hl",
        "ix",
        "iy",
        "sp",
        "pc",
        "af_",
        "bc_",
        "de_",
        "hl_",
        "i",
        "r",
        "im",
        "iff1",
        "iff2",
        "flags",
        "r8_mask",
    ]
    return dict(zip(keys, vals))


def fmt_mask(mask: int) -> str:
    parts = [name for bit, name in MASK_NAMES if mask & bit]
    return ", ".join(parts) if parts else "(none)"


def fmt_state(mask: int, st: dict) -> str:
    parts: list[str] = []
    for bit, name in MASK_NAMES:
        if not (mask & bit):
            continue
        if name == "a":
            parts.append(f"a=0x{(st['af'] >> 8) & 0xFF:02X}")
        elif name == "f":
            parts.append(f"f=0x{st['af'] & 0xFF:02X}")
        elif name == "halted":
            parts.append("halted=1")
        elif name == "trap":
            parts.append("trap=1")
        elif name.startswith("flag."):
            fb = {
                "flag.s": 0x80,
                "flag.z": 0x40,
                "flag.y": 0x20,
                "flag.x": 0x08,
                "flag.h": 0x10,
                "flag.pv": 0x04,
                "flag.n": 0x02,
                "flag.c": 0x01,
            }[name]
            parts.append(f"{name}={1 if st['flags'] & fb else 0}")
        elif name in ("i", "r", "im", "iff1", "iff2"):
            parts.append(f"{name}=0x{st[name]:02X}")
        else:
            parts.append(f"{name}=0x{st[name]:04X}")

    r8 = st.get("r8_mask", 0)
    r8_map = [
        (1 << 0, "b", "bc", 8),
        (1 << 1, "c", "bc", 0),
        (1 << 2, "d", "de", 8),
        (1 << 3, "e", "de", 0),
        (1 << 4, "h", "hl", 8),
        (1 << 5, "l", "hl", 0),
    ]
    for bit, name, pair, shift in r8_map:
        if r8 & bit:
            parts.append(f"{name}=0x{(st[pair] >> shift) & 0xFF:02X}")
    return ", ".join(parts) if parts else "(none)"


def main() -> int:
    ap = argparse.ArgumentParser(description="Dump Z80 test vectors")
    ap.add_argument("vectors", type=Path, nargs="?", default=Path("tests/test_vectors.bin"))
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    data = args.vectors.read_bytes()
    if len(data) < 11 or data[:4] != b"Z80T":
        print("error: invalid or missing magic", file=sys.stderr)
        return 1

    version, count, blob_size = struct.unpack_from("<BHI", data, 4)
    if version != 1:
        print(f"error: unsupported version {version}", file=sys.stderr)
        return 1

    off = 11
    tests = []
    for i in range(count):
        name_len = struct.unpack_from("<H", data, off)[0]
        off += 2
        name = data[off : off + name_len].decode("utf-8")
        off += name_len
        start_pc, end_pc, code_off, code_len, init_mask, expect_mask = struct.unpack_from("<HHIIII", data, off)
        off += 20
        init = unpack_state(data[off : off + 36])
        off += 36
        expect = unpack_state(data[off : off + 36])
        off += 36
        mem_count, io_count = struct.unpack_from("<HH", data, off)
        off += 4
        mem = []
        for _ in range(mem_count):
            addr, val = struct.unpack_from("<HB", data, off)
            off += 3
            mem.append((addr, val))
        io = []
        for _ in range(io_count):
            port, val = struct.unpack_from("<HB", data, off)
            off += 3
            io.append((port, val))
        expect_mem_count = struct.unpack_from("<H", data, off)[0]
        off += 2
        expect_mem = []
        for _ in range(expect_mem_count):
            addr, val = struct.unpack_from("<HB", data, off)
            off += 3
            expect_mem.append((addr, val))
        tests.append(
            {
                "name": name,
                "start_pc": start_pc,
                "end_pc": end_pc,
                "code_off": code_off,
                "code_len": code_len,
                "init_mask": init_mask,
                "expect_mask": expect_mask,
                "init": init,
                "expect": expect,
                "mem": mem,
                "io": io,
                "expect_mem": expect_mem,
            }
        )

    blob_start = off
    code_blob = data[blob_start : blob_start + blob_size]

    print(f"File: {args.vectors}  version={version}  tests={count}  code_bytes={blob_size}")
    print()

    for i, t in enumerate(tests):
        code = code_blob[t["code_off"] : t["code_off"] + t["code_len"]]
        hex_code = " ".join(f"{b:02X}" for b in code)
        print(f"Test {i}: {t['name']}")
        print(f"  start=0x{t['start_pc']:04X} end=0x{t['end_pc']:04X} code_len={t['code_len']}")
        print(f"  init:  {fmt_state(t['init_mask'], t['init'])}")
        if t["mem"]:
            print(f"  mem:   {', '.join(f'0x{a:04X}=0x{v:02X}' for a, v in t['mem'])}")
        else:
            print("  mem:   (none)")
        if t["io"]:
            print(f"  io:    {', '.join(f'0x{p:04X}=0x{v:02X}' for p, v in t['io'])}")
        else:
            print("  io:    (none)")
        print(f"  expect: {fmt_state(t['expect_mask'], t['expect'])}")
        if t["expect_mem"]:
            print(f"  expect_mem: {', '.join(f'0x{a:04X}=0x{v:02X}' for a, v in t['expect_mem'])}")
        print(f"  code:  {hex_code}")
        if args.verbose:
            print(f"  init_mask:   {fmt_mask(t['init_mask'])}")
            print(f"  expect_mask: {fmt_mask(t['expect_mask'])}")
        print()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
