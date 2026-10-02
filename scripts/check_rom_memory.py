"""Check a native GB Studio/GBDK NOI file before running its ROM.

The pinned GB Studio compiler sets .STACK=0xDF00. GBVM absolute.c reserves
the 256-byte page beginning at shadow_OAM2 for sprite DMA, palettes and text
tiles. The native stack grows down from .STACK; linker-allocated WRAM must
end below it. A successful link alone does not enforce this separation.

256 bytes of remaining stack is a project warning threshold, not a measured
maximum stack depth or a hardware certification. Use --min-stack-reserve to
make a chosen reserve mandatory. This check does not execute or modify a ROM.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys


DEFINITION = re.compile(r"^DEF\s+(\S+)\s+(0[xX][0-9a-fA-F]+|[0-9]+)\s*$")
RECOMMENDED_STACK_RESERVE = 256


@dataclass(frozen=True)
class MemoryReport:
    heap_end: int
    stack_base: int
    stack_reserve: int
    errors: tuple[str, ...]
    warnings: tuple[str, ...]


def read_symbols(text: str) -> dict[str, int]:
    """Read the compiler's `DEF symbol 0xADDRESS` NOI format strictly."""
    symbols: dict[str, int] = {}
    for number, line in enumerate(text.splitlines(), 1):
        if not line.startswith("DEF "):
            continue
        match = DEFINITION.fullmatch(line)
        if not match:
            raise ValueError(f"Malformed NOI definition at line {number}: {line}")
        name, raw = match.groups()
        value = int(raw, 16 if raw.lower().startswith("0x") else 10)
        if name in symbols and symbols[name] != value:
            raise ValueError(f"Conflicting NOI definitions for {name}")
        symbols[name] = value
    return symbols


def inspect_memory(symbols: dict[str, int], min_stack_reserve: int = 0) -> MemoryReport:
    required = (
        "s__DATA", "l__DATA", "s__INITIALIZED", "l__INITIALIZED",
        "s__HEAP_END", ".STACK", "_shadow_OAM", "_shadow_OAM2",
        "_BkgPalette", "_vwf_tile_data",
    )
    missing = [name for name in required if name not in symbols]
    if missing:
        raise ValueError("Required native symbols missing: " + ", ".join(missing))
    if min_stack_reserve < 0:
        raise ValueError("Minimum stack reserve cannot be negative")

    heap_end, stack = symbols["s__HEAP_END"], symbols[".STACK"]
    errors: list[str] = []
    warnings: list[str] = []
    if not (0xC000 <= heap_end <= 0xE000 and 0xC000 <= stack <= 0xE000):
        raise ValueError("Heap and native stack boundaries must be in Game Boy WRAM")

    # These are actual stock GBVM absolute allocations, not the linker heap.
    oam2 = symbols["_shadow_OAM2"]
    reserved = (
        ("shadow_OAM", symbols["_shadow_OAM"], 160),
        ("shadow_OAM2", oam2, 160),
        ("BkgPalette", symbols["_BkgPalette"], 64),
        ("vwf_tile_data", symbols["_vwf_tile_data"], 32),
    )
    if symbols["_BkgPalette"] != oam2 + 160 or symbols["_vwf_tile_data"] != oam2 + 224:
        errors.append("Absolute OAM/palette/text layout differs from the inspected stock engine")
    for name, start, length in reserved:
        if start < 0xC000 or start + length > 0xE000:
            errors.append(f"Absolute {name} allocation lies outside WRAM")
        if heap_end < start + length and stack > start:
            errors.append(f"Downward native stack region crosses absolute {name} allocation at {start:04X}")

    allocated: list[tuple[str, int, int]] = []
    for area in ("DATA", "BSS", "INITIALIZED", "HEAP"):
        start_name, length_name = f"s__{area}", f"l__{area}"
        if start_name not in symbols and length_name not in symbols:
            continue
        if start_name not in symbols or length_name not in symbols:
            raise ValueError(f"Incomplete linker area {area}")
        start, length = symbols[start_name], symbols[length_name]
        if not length:
            continue
        end = start + length
        allocated.append((area, start, end))
        if start < 0xC000 or end > 0xE000:
            errors.append(f"{area} [{start:04X},{end:04X}) lies outside WRAM")
        if end > heap_end:
            errors.append(f"{area} ends at {end:04X}, beyond heap marker {heap_end:04X}")
        for name, fixed_start, fixed_length in reserved:
            if start < fixed_start + fixed_length and end > fixed_start:
                errors.append(f"{area} [{start:04X},{end:04X}) overlaps {name} at {fixed_start:04X}")
    for index, (name, start, end) in enumerate(allocated):
        for other, other_start, other_end in allocated[index + 1:]:
            if start < other_end and end > other_start:
                errors.append(f"Linker areas {name} and {other} overlap")

    reserve = stack - heap_end
    if reserve <= 0:
        errors.append(f"Heap end {heap_end:04X} reaches native stack boundary {stack:04X} ({reserve} bytes reserve)")
    elif reserve < min_stack_reserve:
        errors.append(f"Native stack reserve {reserve} bytes is below required {min_stack_reserve}")
    elif reserve < RECOMMENDED_STACK_RESERVE:
        warnings.append(f"Native stack reserve {reserve} bytes is below recommended {RECOMMENDED_STACK_RESERVE}; measure stack usage before release")
    return MemoryReport(heap_end, stack, reserve, tuple(errors), tuple(warnings))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("symbols", type=Path, nargs="+", help="Generated symbols.noi file(s)")
    parser.add_argument("--min-stack-reserve", type=int, default=0,
                        help="Require this many bytes below .STACK; default warns below 256")
    args = parser.parse_args(argv)
    failed = False
    for path in args.symbols:
        try:
            report = inspect_memory(read_symbols(path.read_text()), args.min_stack_reserve)
        except (OSError, ValueError) as exc:
            print(f"FAIL {path}: {exc}", file=sys.stderr)
            failed = True
            continue
        status = "FAIL" if report.errors else "PASS"
        print(f"{status} {path}: heap={report.heap_end:04X}, stack={report.stack_base:04X}, reserve={report.stack_reserve} bytes")
        for error in report.errors:
            print(f"  ERROR: {error}")
        for warning in report.warnings:
            print(f"  WARNING: {warning}")
        failed |= bool(report.errors)
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
