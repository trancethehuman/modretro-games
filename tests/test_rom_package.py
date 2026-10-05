"""Attack-oriented checks of the independent, read-only release audit helpers."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import warnings
import zipfile

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/audit_rom_package.py"
spec = importlib.util.spec_from_file_location("rom_package_audit", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def native_rom():
    data = bytearray(1048576)
    data[0x104:0x134] = audit.LOGO
    data[0x134:0x143] = b"TORONTODISPATCH".ljust(15, b"\0")
    for address, value in ((0x143, 0xC0), (0x147, 0x1E), (0x148, 5), (0x149, 3)):
        data[address] = value
    data[0x14D] = (-sum(data[0x134:0x14D]) - 25) & 255
    struct.pack_into(">H", data, 0x14E, (sum(data[:0x14E]) + sum(data[0x150:])) & 65535)
    return data


class ReleaseAuditTests(unittest.TestCase):
    def test_complete_native_input_scope(self):
        for suffix in ("project.gbsproj", "assets/sprites/car.png", "assets/sprites/car.png.gbsres",
                       "plugins/local/engine/src/game.c", "project/scenes/native.gbsres"):
            self.assertTrue(audit.native_input(audit.PROJECT + "/" + suffix))
        for suffix in ("build/private.sav", "original-art/reference.png", "README.md"):
            self.assertFalse(audit.native_input(audit.PROJECT + "/" + suffix))

    def test_both_frozen_pin_formats(self):
        name = audit.PROJECT + "/project.gbsproj"
        digest = hashlib.sha256(b"native source").hexdigest()
        for document in ({name: digest}, {"files": {name: {"sha256": digest, "sizeBytes": 13}}}):
            pins = audit.pin_files(json.dumps(document).encode())
            self.assertEqual(pins[name]["sha256"], digest)

    def test_missing_project_pin_rejected(self):
        with self.assertRaises(ValueError):
            audit.pin_files(json.dumps({audit.PROJECT + "/assets/a.png": "0" * 64}).encode())

    def test_unsafe_and_invalid_pins_rejected(self):
        name = audit.PROJECT + "/project.gbsproj"
        for document in ({name: "short"}, {name: {"sha256": "0" * 64, "sizeBytes": -1}},
                         {name: 123}, {"../escape": "0" * 64, name: "0" * 64}):
            with self.subTest(document=document), self.assertRaises(ValueError):
                audit.pin_files(json.dumps(document).encode())

    def test_duplicate_json_keys_rejected(self):
        with self.assertRaises(ValueError):
            audit.unique_json(b'{"sourceCommit":"a","sourceCommit":"b"}')

    def test_exact_native_header(self):
        self.assertEqual(audit.header(native_rom())["SRAMBytes"], 32768)

    def test_rom_corruption_and_wrong_mapper_rejected(self):
        original = native_rom()
        for address in (0x104, 0x134, 0x143, 0x147, 0x149, 0x14D, 0x14F, 0x6000):
            changed = bytearray(original)
            changed[address] ^= 1
            with self.subTest(address=address), self.assertRaises(ValueError):
                audit.header(changed)
        with self.assertRaises(ValueError):
            audit.header(original[:-1])

    def write_zip(self, path, extra=None, mode=0o100644):
        names = sorted(["candidate.gbc", "SHA256SUMS", "LICENSE", "ROM_NOTICES.txt", "LOADING.md", "BUILDINFO.json"])
        if extra:
            names.append(extra)
        with zipfile.ZipFile(path, "w") as archive:
            for name in names:
                row = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
                row.compress_type = zipfile.ZIP_DEFLATED
                row.create_system = 3
                row.external_attr = mode << 16
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore", UserWarning)
                    archive.writestr(row, b"inert test data")

    def test_exact_zip_members(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.zip"
            self.write_zip(path)
            self.assertEqual(len(audit.archive_members(path, "candidate.gbc")), 6)

    def test_private_member_and_traversal_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.zip"
            for extra in ("private.sav", "../LICENSE", "LICENSE"):
                with self.subTest(extra=extra):
                    self.write_zip(path, extra)
                    with self.assertRaises(ValueError):
                        audit.archive_members(path, "candidate.gbc")

    def test_zip_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.zip"
            self.write_zip(path, mode=0o120777)
            with self.assertRaises(ValueError):
                audit.archive_members(path, "candidate.gbc")

    def test_zip_payload_corruption_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.zip"
            self.write_zip(path)
            wire = bytearray(path.read_bytes())
            with zipfile.ZipFile(path) as archive:
                first = archive.infolist()[0]
            start = first.header_offset + 30 + len(first.filename.encode())
            wire[start] ^= 0x40
            path.write_bytes(wire)
            with self.assertRaises((ValueError, zipfile.BadZipFile)):
                audit.archive_members(path, "candidate.gbc")

    def test_private_and_secret_text_patterns(self):
        for payload in (b"/Users/example/private", b"http://127.0.0.1:1234/private", b"selectionToken",
                        b"activationCode", b"-----BEGIN PRIVATE KEY-----", b"ghp_abcdefghijklmnopqrst"):
            self.assertIsNotNone(audit.PRIVATE.search(payload))
        self.assertIsNone(audit.PRIVATE.search(b"MIT License\nOpen Government Licence - Toronto\n"))


if __name__ == "__main__":
    unittest.main()
