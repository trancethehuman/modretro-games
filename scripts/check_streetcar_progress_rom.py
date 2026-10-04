"""Check the exact compiled streetcar interpolation data in a matching ROM/NOI.

Independently derives six 721-word floor(length*tick/720) sequences and the
16-entry near-pointer order. Every sequence and the pointer table must occur
exactly once in the whole supplied ROM, fit in one switchable 16-KiB bank, and
share that bank with the linked pose/bounds/sweep APIs and their b_ annotations.

This proves compiled values and placement for the supplied pair. It does not
decode call sites, authenticate that the two files came from the same build,
execute the ROM, measure performance/stack use, or certify physical hardware.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
from pathlib import Path
import sys

from check_rom_memory import read_symbols


BANK_SIZE = 0x4000
LENGTHS = (256, 1600, 1984, 4096, 4864, 10944)
ROUTE_LENGTHS = (4096, 4096, 4864, 4864, 1984, 10944, 1600, 1600,
                10944, 1984, 4864, 4864, 4096, 4096, 256, 256)
APIS = ("td_streetcar_pose", "td_streetcar_bounds", "td_streetcar_sweep")
TICKS = 721
TABLE_BYTES = TICKS * 2
POINTER_BYTES = len(ROUTE_LENGTHS) * 2
NATIVE_DATA_BYTES = len(LENGTHS) * TABLE_BYTES + POINTER_BYTES


@dataclass(frozen=True)
class Location:
    offset: int
    bank: int
    address: int


@dataclass(frozen=True)
class Report:
    bank: int
    tables: tuple[tuple[int, Location], ...]
    routes: Location


def words(length: int) -> bytes:
    """Wide Python arithmetic oracle, independent of generated C literals."""
    return b"".join((length * tick // 720).to_bytes(2, "little")
                    for tick in range(TICKS))


def location(offset: int, length: int, size: int, name: str) -> Location:
    if offset < BANK_SIZE or offset + length > size:
        raise ValueError(f"{name}: resource is outside switchable ROM")
    bank, within = divmod(offset, BANK_SIZE)
    if not 1 <= bank <= 255:
        raise ValueError(f"{name}: bank is outside the native one-byte bank range")
    if within + length > BANK_SIZE:
        raise ValueError(f"{name}: resource crosses its 16-KiB bank boundary")
    return Location(offset, bank, 0x4000 + within)


def unique(rom: bytes, expected: bytes, name: str) -> Location:
    start = rom.find(expected)
    if start < 0:
        raise ValueError(f"{name}: exact compiled bytes are missing or modified")
    if rom.find(expected, start + 1) >= 0:
        raise ValueError(f"{name}: exact compiled bytes are ambiguous (multiple matches)")
    return location(start, len(expected), len(rom), name)


def api_bank(rom: bytes, symbols: dict[str, int], name: str) -> int:
    function, annotation = "_" + name, "b_" + name
    for required in (function, annotation):
        if required not in symbols:
            raise ValueError(f"Required linked API symbol missing: {required}")
    bank, address = divmod(symbols[function], 0x10000)
    if not 1 <= bank <= 255 or not 0x4000 <= address < 0x8000:
        raise ValueError(f"{function}: invalid switchable ROM symbol")
    if symbols[annotation] != bank:
        raise ValueError(f"{annotation}: bank annotation disagrees with {function}")
    location(bank * BANK_SIZE + address - 0x4000, 1, len(rom), function)
    return bank


def inspect(rom: bytes, symbols: dict[str, int]) -> Report:
    if len(rom) < 2 * BANK_SIZE or len(rom) % BANK_SIZE:
        raise ValueError("ROM must contain complete 16-KiB banks, including a switchable bank")
    banks = tuple(api_bank(rom, symbols, name) for name in APIS)
    if len(set(banks)) != 1:
        raise ValueError("Linked pose/bounds/sweep APIs do not share one ROM bank")
    bank = banks[0]
    tables = tuple((length, unique(rom, words(length), f"progress[{length}]"))
                   for length in LENGTHS)
    for length, table in tables:
        if table.bank != bank:
            raise ValueError(f"progress[{length}]: table bank {table.bank} differs from API bank {bank}")
    by_length = dict(tables)
    expected = b"".join(by_length[length].address.to_bytes(2, "little")
                        for length in ROUTE_LENGTHS)
    routes = unique(rom, expected, "16-route near-pointer table")
    if routes.bank != bank:
        raise ValueError(f"16-route near-pointer table: bank {routes.bank} differs from API bank {bank}")
    # The table's exact byte order identifies each full sequence, rather than
    # accepting arbitrary in-bank word pointers or the same addresses in another bank.
    return Report(bank, tables, routes)


def fixture() -> tuple[bytearray, dict[str, int], dict[int, int], int]:
    rom = bytearray(b"\xA5" * (4 * BANK_SIZE))
    tables = {length: 2 * BANK_SIZE + 0x300 + index * 0x600
              for index, length in enumerate(LENGTHS)}
    for length, start in tables.items():
        rom[start:start + TABLE_BYTES] = words(length)
    routes = 2 * BANK_SIZE + 0x2800
    rom[routes:routes + POINTER_BYTES] = b"".join(
        (0x4000 + tables[length] % BANK_SIZE).to_bytes(2, "little")
        for length in ROUTE_LENGTHS)
    symbols = {}
    for index, name in enumerate(APIS):
        symbols["_" + name] = 2 << 16 | (0x4100 + index * 0x20)
        symbols["b_" + name] = 2
    return rom, symbols, tables, routes


def rewrite_routes(rom: bytearray, tables: dict[int, int], start: int) -> None:
    rom[start:start + POINTER_BYTES] = b"".join(
        (0x4000 + tables[length] % BANK_SIZE).to_bytes(2, "little")
        for length in ROUTE_LENGTHS)


def self_test() -> int:
    checks = 0

    def accepted(rom: bytes, symbols: dict[str, int]) -> Report:
        nonlocal checks
        report = inspect(bytes(rom), symbols)
        checks += 1
        return report

    def rejected(rom: bytes, symbols: dict[str, int], reason: str) -> None:
        nonlocal checks
        try:
            inspect(bytes(rom), symbols)
        except ValueError as error:
            if reason not in str(error):
                raise AssertionError(f"Expected {reason!r}, got {error!r}") from error
            checks += 1
        else:
            raise AssertionError(f"Mutation incorrectly passed: {reason}")

    rom, symbols, tables, routes = fixture()
    report = accepted(rom, symbols)
    assert report.bank == 2 and len(report.tables) == 6
    assert report.routes.address == 0x6800 and NATIVE_DATA_BYTES == 8684
    checks += 2

    # Every value (including exact start/end) and every route pointer is tested
    # in isolation; a duplicated valid table cannot mask a damaged original.
    for length in LENGTHS:
        for tick in range(TICKS):
            damaged = rom.copy()
            damaged[tables[length] + tick * 2] ^= 1
            rejected(damaged, symbols, f"progress[{length}]: exact compiled bytes are missing")
        for duplicate in (BANK_SIZE + 0x1200, 2 * BANK_SIZE + 0x3000):
            damaged = rom.copy()
            damaged[duplicate:duplicate + TABLE_BYTES] = words(length)
            rejected(damaged, symbols, f"progress[{length}]: exact compiled bytes are ambiguous")
        damaged = rom.copy()
        damaged[tables[length]:tables[length] + TABLE_BYTES] = b"\xA5" * TABLE_BYTES
        rejected(damaged, symbols, f"progress[{length}]: exact compiled bytes are missing")

    for index in range(len(ROUTE_LENGTHS)):
        for value in (0, 0x3FFF, 0x8000, 0x4301,
                      0x4000 + tables[next(length for length in LENGTHS
                                          if length != ROUTE_LENGTHS[index])] % BANK_SIZE):
            damaged = rom.copy()
            damaged[routes + index * 2:routes + index * 2 + 2] = value.to_bytes(2, "little")
            rejected(damaged, symbols, "16-route near-pointer table: exact compiled bytes are missing")
    for duplicate in (BANK_SIZE + 0x1200, 2 * BANK_SIZE + 0x3000):
        damaged = rom.copy()
        damaged[duplicate:duplicate + POINTER_BYTES] = rom[routes:routes + POINTER_BYTES]
        rejected(damaged, symbols, "16-route near-pointer table: exact compiled bytes are ambiguous")

    # Exact tables/map in another bank are insufficient even if all 16 near
    # addresses still look valid; near pointers inherit the API's current bank.
    for length in LENGTHS:
        damaged = rom.copy()
        relocated = tables[length] + BANK_SIZE
        damaged[relocated:relocated + TABLE_BYTES] = words(length)
        damaged[tables[length]:tables[length] + TABLE_BYTES] = b"\xA5" * TABLE_BYTES
        rejected(damaged, symbols, f"progress[{length}]: table bank 3 differs")
    damaged = rom.copy()
    damaged[routes + BANK_SIZE:routes + BANK_SIZE + POINTER_BYTES] = rom[routes:routes + POINTER_BYTES]
    damaged[routes:routes + POINTER_BYTES] = b"\xA5" * POINTER_BYTES
    rejected(damaged, symbols, "16-route near-pointer table: bank 3 differs")

    # Resource placements at the exact end of a bank pass; one byte beyond
    # that end fails, without truncating the pattern or corrupting other tables.
    for length in LENGTHS:
        for over in (0, 1):
            damaged = rom.copy()
            starts = tables.copy()
            starts[length] = 3 * BANK_SIZE - TABLE_BYTES + over
            damaged[tables[length]:tables[length] + TABLE_BYTES] = b"\xA5" * TABLE_BYTES
            damaged[starts[length]:starts[length] + TABLE_BYTES] = words(length)
            rewrite_routes(damaged, starts, routes)
            if over:
                rejected(damaged, symbols, f"progress[{length}]: resource crosses")
            else:
                accepted(damaged, symbols)
    for over in (0, 1):
        damaged = rom.copy()
        start = 3 * BANK_SIZE - POINTER_BYTES + over
        damaged[routes:routes + POINTER_BYTES] = b"\xA5" * POINTER_BYTES
        rewrite_routes(damaged, tables, start)
        if over:
            rejected(damaged, symbols, "16-route near-pointer table: resource crosses")
        else:
            accepted(damaged, symbols)

    for name in APIS:
        for missing in ("_" + name, "b_" + name):
            changed = symbols.copy()
            del changed[missing]
            rejected(rom, changed, "Required linked API symbol missing")
        changed = symbols.copy()
        changed["b_" + name] = 3
        rejected(rom, changed, "bank annotation disagrees")
        changed = symbols.copy()
        changed["_" + name] = 3 << 16 | 0x4100
        rejected(rom, changed, "bank annotation disagrees")
        changed["b_" + name] = 3
        rejected(rom, changed, "APIs do not share")
        for invalid in (0x4100, 2 << 16 | 0x3FFF, 2 << 16 | 0x8000, 256 << 16 | 0x4100):
            changed = symbols.copy()
            changed["_" + name] = invalid
            rejected(rom, changed, "invalid switchable ROM symbol")
    changed = {key: (4 if key.startswith("b_") else 4 << 16 | 0x4100)
               for key in symbols}
    rejected(rom, changed, "outside switchable ROM")
    for truncated in (rom[:BANK_SIZE], rom[:-1]):
        rejected(truncated, symbols, "complete 16-KiB banks")
    rejected(rom[:3 * BANK_SIZE], changed, "outside switchable ROM")

    # Fixed-bank patterns are not accepted as near-pointer resources.
    damaged = rom.copy()
    length = LENGTHS[0]
    damaged[0x1000:0x1000 + TABLE_BYTES] = words(length)
    damaged[tables[length]:tables[length] + TABLE_BYTES] = b"\xA5" * TABLE_BYTES
    rejected(damaged, symbols, "outside switchable ROM")
    for text, reason in (("DEF b_td_streetcar_pose nope", "Malformed NOI"),
                         ("DEF b_td_streetcar_pose 0x2\nDEF b_td_streetcar_pose 0x3", "Conflicting NOI")):
        try:
            read_symbols(text)
        except ValueError as error:
            assert reason in str(error)
            checks += 1
        else:
            raise AssertionError("Malformed/conflicting NOI was accepted")
    parsed = read_symbols("\n".join(f"DEF {name} 0x{value:X}" for name, value in symbols.items()))
    accepted(rom, parsed)
    print(f"PASS: {checks} compiled progress guard fixture checks, including isolated value/pointer mutations")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, help="Official compiled .gbc ROM")
    parser.add_argument("--noi", type=Path, help="symbols.noi from that exact build")
    parser.add_argument("--self-test", action="store_true", help="Run isolated synthetic ROM/NOI fixtures")
    args = parser.parse_args(argv)
    if args.self_test:
        if args.rom or args.noi:
            parser.error("--self-test cannot be combined with --rom/--noi")
        return self_test()
    if not args.rom or not args.noi:
        parser.error("supply both --rom and --noi, or use --self-test")
    try:
        rom, noi = args.rom.read_bytes(), args.noi.read_bytes()
    except OSError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print(f"ROM SHA-256 {hashlib.sha256(rom).hexdigest()}")
    print(f"NOI SHA-256 {hashlib.sha256(noi).hexdigest()}")
    try:
        report = inspect(rom, read_symbols(noi.decode("utf-8")))
    except (UnicodeError, ValueError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    for length, table in report.tables:
        print(f"  progress[{length}]: bank {table.bank:02X}, near {table.address:04X}, {TABLE_BYTES} bytes")
    print(f"  route pointers: bank {report.routes.bank:02X}, near {report.routes.address:04X}, {POINTER_BYTES} bytes")
    print(f"PASS: six exact 721-word progress tables and 16 ordered near pointers ({NATIVE_DATA_BYTES} bytes), "
          f"unique in ROM and co-banked with linked pose/bounds/sweep APIs in bank {report.bank:02X}")
    print("Compiled data/placement only; native execution, pacing, stack use and hardware remain separate checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
