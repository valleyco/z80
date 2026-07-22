#!/usr/bin/env python3
"""Generate z80_simple*.json from z80_instruction_set.json with flags from Zilog semantics."""

import json
from copy import deepcopy
from pathlib import Path

FLAGS_ORDER = ["S", "Z", "F5", "H", "F3", "P/V", "N", "C"]
NONE = ["-"] * 8


def f(s="-", z="-", f5="-", h="-", f3="-", pv="-", n="-", c="-"):
    return [s, z, f5, h, f3, pv, n, c]


# Flag presets (Zilog Z80 CPU User Manual / standard references)
FLAGS = {
    "none": NONE,
    "inc8": f(s="S", z="Z", h="H", pv="P/V", n="0"),
    "dec8": f(s="S", z="Z", h="H", pv="P/V", n="1"),
    "add_a": f(s="S", z="Z", h="H", pv="P/V", n="0", c="C"),
    "adc_a": f(s="S", z="Z", h="H", pv="P/V", n="0", c="C"),
    "sub_a": f(s="S", z="Z", h="H", pv="P/V", n="1", c="C"),
    "sbc_a": f(s="S", z="Z", h="H", pv="P/V", n="1", c="C"),
    "and_a": f(s="S", z="Z", h="1", pv="P/V", n="0", c="0"),
    "xor_a": f(s="S", z="Z", h="0", pv="P/V", n="0", c="0"),
    "or_a": f(s="S", z="Z", h="0", pv="P/V", n="0", c="0"),
    "cp_a": f(s="S", z="Z", h="H", pv="P/V", n="1", c="C"),
    "add_hl": f(h="H", n="0", c="C"),
    "rotate_a": f(c="C"),
    "daa": f(s="S", z="Z", h="H", c="C"),
    "cpl": f(h="1", n="1"),
    "scf": f(n="0", h="0", c="1"),
    "ccf": f(n="0", h="C", c="?"),
    "cb_rotate": f(s="S", z="Z", h="0", pv="P/V", n="0", c="C"),
    "cb_shift": f(s="S", z="Z", pv="P/V", n="0", c="C"),
    "cb_sra": f(s="S", z="Z", h="0", pv="P/V", n="0", c="C"),
    "bit": f(s="S", z="Z", h="1", pv="P/V", n="0", c="0"),
    "neg": f(s="S", z="Z", h="H", pv="P/V", n="1", c="C"),
    "adc_hl": f(h="H", pv="P/V", n="0", c="C"),
    "sbc_hl": f(h="H", pv="P/V", n="1", c="C"),
    "in_reg_c": f(s="S", z="Z", h="H", pv="P/V", n="0", c="C"),
    "ld_a_i_r": f(s="S", z="Z", h="H", pv="?", n="0"),
    "ldi_ldd": f(s="0", h="0", pv="P/V", n="0"),
    "cpi_cpd": f(s="S", z="Z", h="H", pv="P/V", n="1", c="C"),
    "block_io": f(n="1", c="C", h="H", pv="P/V"),
    "rrd_rld": f(s="S", z="Z", h="H", pv="P/V", n="0", c="C"),
}


def normalize_mnemonic(mnemonic: str) -> str:
    return mnemonic.replace("{", "").replace("}", "")


def normalize_opcode(pattern: str) -> str:
    return pattern.replace("BBB", "bbb")


def map_operands(operands: list, mnemonic: str, bytes_count: int | None) -> list:
    """Map to Wikipedia-style operand names (immediate bytes; CB bit ops include bbb/sss)."""
    m = mnemonic.lower()
    include_reg_fields = m.startswith(("bit ", "res ", "set "))
    result = []
    for op in operands:
        key = op.lower()
        if key == "data":
            if "rp" in m and "data" in m:
                return ["datlo", "dathi"]
            result.append("data")
            continue
        if key == "add":
            return ["addlo", "addhi"]
        if key in ("port", "offset"):
            result.append(key)
            continue
        if key in ("b", "bbb"):
            if include_reg_fields:
                result.append("bbb")
            continue
        if key in ("r", "sss"):
            if include_reg_fields:
                result.append("sss")
            continue
    return result


def flags_for_entry(entry: dict) -> list:
    mnemonic = entry["mnemonic"]
    prefix = entry.get("prefix")
    m = normalize_mnemonic(mnemonic).upper()

    if m in ("NOP", "HALT", "EX AF,AF'", "EXX", "EX DE,HL", "EX (SP),HL", "DI", "EI"):
        return FLAGS["none"]
    if m.startswith("LD ") and "A," not in m and "ADD" not in m:
        if "I" in m or "R" in m:
            if m in ("LD I,A", "LD R,A"):
                return FLAGS["none"]
        return FLAGS["none"]
    if m.startswith("POP ") or m.startswith("PUSH "):
        return FLAGS["none"]
    if m.startswith("JP ") or m.startswith("JR ") or m == "JP (HL)":
        return FLAGS["none"]
    if m.startswith("CALL ") or m.startswith("RET") or m.startswith("RST "):
        return FLAGS["none"]
    if m.startswith("OUT ") or m.startswith("IN A,"):
        return FLAGS["none"]
    if m in ("CB PREFIX", "ED PREFIX", "IX OVERRIDE", "IY OVERRIDE"):
        return FLAGS["none"]
    if m.startswith("IM "):
        return FLAGS["none"]
    if m.startswith("LD SP,HL"):
        return FLAGS["none"]

    if m.startswith("INC {RP}") or normalize_mnemonic(mnemonic).startswith("INC rp"):
        return FLAGS["none"]
    if m.startswith("DEC {RP}") or normalize_mnemonic(mnemonic).startswith("DEC rp"):
        return FLAGS["none"]
    if m.startswith("INC ") or m.startswith("INC{"):
        return FLAGS["inc8"]
    if m.startswith("DEC "):
        return FLAGS["dec8"]

    if m.startswith("ADD HL,"):
        return FLAGS["add_hl"]
    if m in ("RLCA", "RRCA", "RLA", "RRA"):
        return FLAGS["rotate_a"]
    if m == "DAA":
        return FLAGS["daa"]
    if m == "CPL":
        return FLAGS["cpl"]
    if m == "SCF":
        return FLAGS["scf"]
    if m == "CCF":
        return FLAGS["ccf"]
    if m == "DJNZ":
        return FLAGS["none"]

    if prefix == "CB":
        if m.startswith("BIT"):
            return FLAGS["bit"]
        if m.startswith("RES") or m.startswith("SET"):
            return FLAGS["none"]
        if m.startswith("SRA"):
            return FLAGS["cb_sra"]
        return FLAGS["cb_rotate"]

    if prefix == "ED":
        if m.startswith("IN ") and "(C)" in m:
            return FLAGS["in_reg_c"]
        if m.startswith("OUT (C)"):
            return FLAGS["none"]
        if m.startswith("NEG"):
            return FLAGS["neg"]
        if m.startswith("ADC HL,"):
            return FLAGS["adc_hl"]
        if m.startswith("SBC HL,"):
            return FLAGS["sbc_hl"]
        if m.startswith("LD A,I") or m.startswith("LD A,R"):
            return FLAGS["ld_a_i_r"]
        if m.startswith("RRD") or m.startswith("RLD"):
            return FLAGS["rrd_rld"]
        if "LDI" in m or "LDD" in m:
            return FLAGS["ldi_ldd"]
        if "CPI" in m or "CPD" in m:
            return FLAGS["cpi_cpd"]
        if m in ("INI", "INIR", "IND", "INDR", "OUTI", "OTIR", "OUTD", "OTDR"):
            return FLAGS["block_io"]
        return FLAGS["none"]

    if "{ALU}" in m or "ALU" in entry.get("fields", {}):
        alu = entry.get("fields", {}).get("alu") == "alu_op"
        if alu or "{alu}" in mnemonic.lower():
            # Pattern instruction - flags depend on ALU field; use generic ALU add pattern
            # Handled below via mnemonic pattern
            pass

    if m.startswith("ADD A,") or m == "{ALU} A,{SSS}" or "ADD" in m:
        if "CP" in m:
            return FLAGS["cp_a"]
    if "CP A," in m or m.endswith("CP A,{SSS}"):
        return FLAGS["cp_a"]

    # ALU pattern instructions {alu} A,{sss} and {alu} A,{data}
    if "{alu}" in mnemonic.lower():
        return FLAGS["add_a"]  # default; overridden per-op below

    return FLAGS["none"]


def flags_for_alu_pattern(entry: dict) -> list:
    """ALU subfield selects op; H/N/C behavior varies — use + where ALU-dependent."""
    return f(s="+", z="+", h="+", pv="+", n="+", c="+")


def transform_entry(entry: dict, section: str) -> dict:
    entry_for_flags = dict(entry)
    if section == "cb":
        entry_for_flags["prefix"] = "CB"
    elif section == "ed":
        entry_for_flags["prefix"] = "ED"

    mnemonic = normalize_mnemonic(entry["mnemonic"])
    # Normalize register placeholders in mnemonics to Wikipedia lowercase
    mnemonic = (
        mnemonic.replace(" rp", " rp")
        .replace(" ddd", " ddd")
        .replace(" sss", " sss")
    )
    if mnemonic.startswith("BIT "):
        mnemonic = "BIT bbb,sss"
    elif mnemonic.startswith("RES "):
        mnemonic = "RES bbb,sss"
    elif mnemonic.startswith("SET "):
        mnemonic = "SET bbb,sss"
    elif section == "cb":
        for op in ("RLC", "RRC", "RL", "RR", "SLA", "SRA", "SRL"):
            if mnemonic.upper().startswith(op):
                mnemonic = f"{op} sss"
                break

    operand = map_operands(entry.get("operands", []), entry["mnemonic"], entry.get("bytes"))

    flags = flags_for_entry(entry_for_flags)
    if "{alu}" in entry["mnemonic"].lower():
        flags = flags_for_alu_pattern(entry)

    return {
        "mnemonic": mnemonic,
        "opcode": normalize_opcode(entry["opcode_pattern"]),
        "operand": operand,
        "flags_effected": flags,
        "description": entry["description"],
    }


def expand_ed_variants(entry: dict) -> list[dict]:
    """Split combined ED block mnemonics into separate entries per Wikipedia."""
    variants = entry.get("variants")
    if not variants:
        return [transform_entry(entry, "ed")]

    results = []
    base = deepcopy(entry)
    del base["variants"]
    pattern = entry["opcode_pattern"]

    for name, info in variants.items():
        e = deepcopy(base)
        e["mnemonic"] = name
        e["opcode_hex"] = info.get("opcode_hex")
        r_bit = str(info["R"])
        d_bit = str(info["D"])
        op_pat = pattern.replace("R", r_bit).replace("D", d_bit)
        e["opcode_pattern"] = op_pat
        e["operands"] = []
        results.append(transform_entry(e, "ed"))
    return results


def write_json(path: Path, payload: dict) -> None:
    with path.open("w") as f:
        json.dump(payload, f, indent=2)
        f.write("\n")


def validate_instructions(instructions: list[dict], label: str) -> None:
    for i, inst in enumerate(instructions):
        assert len(inst["flags_effected"]) == 8, f"{label} {i} {inst['mnemonic']}: bad flags length"


def main():
    base = Path(__file__).parent
    src = base / "z80_instruction_set.json"

    with src.open() as f:
        data = json.load(f)

    root_instructions = [
        transform_entry(entry, "root") for entry in data["instructions"]["root"]
    ]

    ed_instructions = []
    for entry in data["instructions"]["ed"]:
        if entry.get("variants"):
            ed_instructions.extend(expand_ed_variants(entry))
        else:
            ed_instructions.append(transform_entry(entry, "ed"))

    cb_instructions = [
        transform_entry(entry, "cb") for entry in data["instructions"]["cb"]
    ]

    source = "https://en.wikipedia.org/wiki/Z80_instruction_set"
    write_json(
        base / "z80_simple.json",
        {
            "source": source,
            "flags_order": FLAGS_ORDER,
            "instructions": root_instructions,
        },
    )
    write_json(
        base / "z80_simple_cb.json",
        {
            "source": source,
            "prefix": "CB",
            "flags_order": FLAGS_ORDER,
            "instructions": cb_instructions,
        },
    )
    write_json(
        base / "z80_simple_ed.json",
        {
            "source": source,
            "prefix": "ED",
            "flags_order": FLAGS_ORDER,
            "instructions": ed_instructions,
        },
    )

    validate_instructions(root_instructions, "root")
    validate_instructions(cb_instructions, "cb")
    validate_instructions(ed_instructions, "ed")

    print(f"Wrote {len(root_instructions)} root -> {base / 'z80_simple.json'}")
    print(f"Wrote {len(cb_instructions)} cb   -> {base / 'z80_simple_cb.json'}")
    print(f"Wrote {len(ed_instructions)} ed   -> {base / 'z80_simple_ed.json'}")


if __name__ == "__main__":
    main()
