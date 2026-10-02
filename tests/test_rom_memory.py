"""Regression cases for the post-build WRAM/OAM/stack guard."""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from check_rom_memory import inspect_memory, read_symbols


def native_symbols(heap_end=0xDD75):
    # Same allocation ordering and absolute page as the inspected native NOI.
    initialized_length = 0x2F
    initialized_start = heap_end - initialized_length
    return {
        "s__DATA": 0xC0A0, "l__DATA": initialized_start - 0xC0A0,
        "s__BSS": initialized_start, "l__BSS": 0,
        "s__INITIALIZED": initialized_start, "l__INITIALIZED": initialized_length,
        "s__HEAP": heap_end, "l__HEAP": 0, "s__HEAP_END": heap_end,
        ".STACK": 0xDF00, "_shadow_OAM": 0xC000, "_shadow_OAM2": 0xDF00,
        "_BkgPalette": 0xDFA0, "_vwf_tile_data": 0xDFE0,
    }


class MemoryGuardTests(unittest.TestCase):
    def test_known_good_layout_has_395_bytes(self):
        report = inspect_memory(native_symbols(), 256)
        self.assertEqual(report.stack_reserve, 395)
        self.assertEqual(report.errors, ())
        self.assertEqual(report.warnings, ())

    def test_observed_failed_district_layout_rejects_oam_overlap(self):
        report = inspect_memory(native_symbols(0xDF90))
        self.assertEqual(report.stack_reserve, -144)
        self.assertTrue(any("overlaps shadow_OAM2" in error for error in report.errors))
        self.assertTrue(any("native stack boundary" in error for error in report.errors))

    def test_exact_stack_boundary_fails_even_without_oam_overlap(self):
        report = inspect_memory(native_symbols(0xDF00))
        self.assertTrue(report.errors)

    def test_small_positive_reserve_warns_or_fails_requested_guard(self):
        report = inspect_memory(native_symbols(0xDEF0))
        self.assertEqual(report.errors, ())
        self.assertTrue(report.warnings)
        self.assertTrue(inspect_memory(native_symbols(0xDEF0), 256).errors)

    def test_initial_sprite_dma_buffer_overlap_is_rejected(self):
        symbols = native_symbols()
        symbols["s__DATA"] = 0xC09F
        symbols["l__DATA"] += 1
        self.assertTrue(any("overlaps shadow_OAM at C000" in error
                            for error in inspect_memory(symbols).errors))

    def test_moving_stack_into_reserved_page_is_not_a_fix(self):
        symbols = native_symbols()
        symbols[".STACK"] = 0xE000
        self.assertTrue(any("stack region crosses absolute shadow_OAM2" in error
                            for error in inspect_memory(symbols).errors))

    def test_heap_marker_cannot_hide_allocation_past_stack(self):
        symbols = native_symbols()
        symbols["l__INITIALIZED"] = 0x200
        report = inspect_memory(symbols)
        self.assertTrue(any("beyond heap marker" in error for error in report.errors))
        self.assertTrue(any("overlaps shadow_OAM2" in error for error in report.errors))

    def test_missing_or_conflicting_symbols_fail_closed(self):
        with self.assertRaises(ValueError):
            inspect_memory({".STACK": 0xDF00})
        with self.assertRaises(ValueError):
            read_symbols("DEF .STACK 0xDF00\nDEF .STACK 0xE000\n")
        with self.assertRaises(ValueError):
            read_symbols("DEF .STACK not-an-address\n")

    def test_actual_noi_syntax_handles_banked_and_dotted_symbols(self):
        symbols = read_symbols("LOAD rom.gbc\nDEF _td 0xC1F6\nDEF .STACK 0xDF00\nDEF _toronto_init 0xB4ABC\n")
        self.assertEqual(symbols["_td"], 0xC1F6)
        self.assertEqual(symbols[".STACK"], 0xDF00)
        self.assertEqual(symbols["_toronto_init"], 0xB4ABC)


if __name__ == "__main__":
    unittest.main()
