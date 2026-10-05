#!/usr/bin/env python3
"""Independently audit a Toronto loading ZIP and frozen native source inputs.

Reads immutable Git objects, the working source and supplied files. Writes only
JSON to stdout. Does not package, rebuild, emulate, install or certify hardware.
Frozen pins need an independently retained pre-build SHA-256; this check does
not establish source-to-ROM correspondence by rebuilding the declared source.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath
import re
import struct
import subprocess
from urllib.parse import quote, unquote, urlsplit
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
GAME = "games/toronto-dispatch"
PROJECT = GAME + "/project"
URL = "https://github.com/trancethehuman/modretro-games"
SCOPES = tuple(PROJECT + "/" + name + "/" for name in ("assets", "plugins", "project"))
LOGO = bytes.fromhex("ceed6666cc0d000b03730083000c000d0008111f8889000edccc6ee6ddddd999bbbb67636e0eecccdddc999fbbb9333e")
PRIVATE = re.compile(rb"/Users/|/home/|/opt/homebrew/|/private/|https?://(?:localhost|127\.0\.0\.1)(?::|/)|data:image/|state\.bin|activationCode|selectionToken|checkpointPath|BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY|github_pat_[A-Za-z0-9_]+|gh[pousr]_[A-Za-z0-9]+|sk-[A-Za-z0-9]{20,}", re.I)
LINK = re.compile(r'(?P<label>!?\[[^\]\n]*\])\((?P<target><[^>\n]+>|[^\s)]+)(?P<title>\s+"[^"\n]*")?\)')
BASELINES = {
    "toronto-dispatch-hardware-feedback.gbc": (1048576, "9c155a70c0cc3d6ccec986fc0ab7ddc3a204fd04e80879ad0233926156d4b10e"),
    "toronto-dispatch-hardware-feedback-reviewed.zip": (155795, "9dd43b59841a2bc5dba399ba88722fabddff1f835f05d32d0048e13628d9b570"),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)


def unique_json(data: bytes):
    def pairs(rows):
        out = {}
        for key, value in rows:
            require(key not in out, "Duplicate JSON key")
            out[key] = value
        return out
    return json.loads(data, object_pairs_hook=pairs)


def native_input(path: str) -> bool:
    return path == PROJECT + "/project.gbsproj" or path.startswith(SCOPES)


def pin_files(raw: bytes) -> dict:
    document = unique_json(raw)
    require(isinstance(document, dict), "Source pins must be an object")
    rows = document.get("files", document)
    require(isinstance(rows, dict) and rows, "Empty source pins")
    out = {}
    for path, value in rows.items():
        require(isinstance(path, str) and not path.startswith("/") and
                ".." not in PurePosixPath(path).parts, "Unsafe pin path")
        if not native_input(path):
            continue
        require(isinstance(value, (str, dict)), "Invalid source pin record")
        digest = value if isinstance(value, str) else value.get("sha256")
        require(isinstance(digest, str) and re.fullmatch(r"[0-9a-f]{64}", digest) is not None,
                "Invalid source pin digest")
        size = None if isinstance(value, str) else value.get("sizeBytes")
        require(size is None or type(size) is int and size >= 0, "Invalid source pin size")
        out[path] = {"sha256": digest, "sizeBytes": size}
    require(out and PROJECT + "/project.gbsproj" in out, "Project file missing from native pins")
    return out


def git_read(*args: str, input_: bytes | None = None) -> bytes:
    require(args[0] in ("cat-file", "ls-tree"), "Only immutable Git reads allowed")
    result = subprocess.run(["git", "-C", str(ROOT), *args], input=input_, capture_output=True)
    require(result.returncode == 0, "Immutable Git object read failed")
    return result.stdout


def committed(commit: str, path: str) -> bytes:
    require(not path.startswith("/") and ".." not in PurePosixPath(path).parts,
            "Unsafe committed path")
    return git_read("cat-file", "blob", commit + ":" + path)


def source_inputs(commit: str) -> dict[str, bytes]:
    rows = git_read("ls-tree", "-r", "-z", commit, "--", PROJECT)
    files = {}
    for row in rows.split(b"\0"):
        if not row:
            continue
        metadata, path = row.split(b"\t", 1)
        path = path.decode()
        if not native_input(path):
            continue
        mode, kind, oid = metadata.decode().split()
        require(kind == "blob" and mode in ("100644", "100755"), "Nonregular native Git input")
        files[path] = oid
    require(files, "No committed native inputs")
    oids = list(dict.fromkeys(files.values()))
    wire = git_read("cat-file", "--batch", input_=("\n".join(oids) + "\n").encode())
    blobs = {}
    offset = 0
    for oid in oids:
        end = wire.index(b"\n", offset)
        actual, kind, size = wire[offset:end].decode().split()
        size = int(size)
        blob = wire[end + 1:end + 1 + size]
        require(actual == oid and kind == "blob" and len(blob) == size and
                wire[end + 1 + size:end + 2 + size] == b"\n", "Malformed batch blob")
        require(hashlib.sha1(b"blob " + str(size).encode() + b"\0" + blob).hexdigest() == oid,
                "Git blob authentication failed")
        blobs[oid] = blob
        offset = end + 2 + size
    require(offset == len(wire), "Trailing batch object bytes")
    return {path: blobs[oid] for path, oid in files.items()}


def header(rom: bytes) -> dict:
    require(len(rom) == 1048576, "Expected a 1MiB Toronto native ROM")
    require(rom[0x104:0x134] == LOGO and rom[0x134:0x143].rstrip(b"\0") == b"TORONTODISPATCH",
            "Native logo/title mismatch")
    require(tuple(rom[i] for i in (0x143, 0x147, 0x148, 0x149)) == (0xC0, 0x1E, 5, 3),
            "Expected CGB-only MBC5/rumble/battery 32KiB SRAM header")
    check = (-sum(rom[0x134:0x14D]) - 25) & 255
    total = (sum(rom[:0x14E]) + sum(rom[0x150:])) & 65535
    require(rom[0x14D] == check and struct.unpack_from(">H", rom, 0x14E)[0] == total,
            "Native header/global checksum mismatch")
    return {"headerChecksum": check, "globalChecksum": total, "CGBOnly": True,
            "ROMBytes": len(rom), "SRAMBytes": 32768}


def archive_members(path: Path, rom_name: str) -> dict[str, bytes]:
    allow = sorted([rom_name, "SHA256SUMS", "LICENSE", "ROM_NOTICES.txt", "LOADING.md", "BUILDINFO.json"])
    with zipfile.ZipFile(path) as archive:
        require(archive.namelist() == allow and len(archive.infolist()) == 6 and not archive.comment,
                "ZIP must contain only six unique sorted root members")
        for row in archive.infolist():
            require(not row.is_dir() and row.filename == PurePosixPath(row.filename).name and
                    row.compress_type == zipfile.ZIP_DEFLATED and row.external_attr >> 16 == 0o100644 and
                    row.date_time == (1980, 1, 1, 0, 0, 0) and not row.extra and not row.comment and
                    row.file_size <= 8 * 1024 * 1024, "Unsafe/nondeterministic ZIP member")
        require(archive.testzip() is None, "ZIP member CRC failed")
        return {name: archive.read(name) for name in allow}


def audit(args: argparse.Namespace) -> dict:
    commit = args.source_commit
    require(re.fullmatch(r"[0-9a-f]{40}", commit) is not None and
            git_read("cat-file", "-t", commit).strip() == b"commit", "Full real source commit required")
    for digest in (args.expected_sha256, args.expected_zip_sha256, args.expected_pins_sha256):
        require(re.fullmatch(r"[0-9a-f]{64}", digest) is not None, "Full expected SHA-256 required")
    for path in (args.rom, args.zip, args.source_pins):
        require(path.is_file() and not path.is_symlink(), "Regular local audit input required")
    require(args.rom.resolve().is_relative_to(ROOT / PROJECT / "build") and
            re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*\.gbc", args.rom.name) is not None,
            "Native ROM must be in this game's ignored build directory")
    rom = args.rom.read_bytes()
    archive = args.zip.read_bytes()
    pin_wire = args.source_pins.read_bytes()
    require(sha(rom) == args.expected_sha256 and sha(archive) == args.expected_zip_sha256 and
            sha(pin_wire) == args.expected_pins_sha256, "Supplied artifact/frozen-pin digest mismatch")
    pins = pin_files(pin_wire)
    blobs = source_inputs(commit)
    require(set(pins) == set(blobs), "Frozen pins must cover the complete committed native input set")
    working = {str(path.relative_to(ROOT)) for scope in SCOPES for path in (ROOT / scope).rglob("*") if path.is_file()}
    working.add(PROJECT + "/project.gbsproj")
    require(working == set(pins), "Working native inputs added/deleted since source freeze")
    counts = Counter()
    for path, blob in blobs.items():
        expected = pins[path]
        working_path = ROOT / path
        require(not working_path.is_symlink() and working_path.read_bytes() == blob,
                "Working/committed native input mismatch: " + path)
        require(sha(blob) == expected["sha256"] and
                (expected["sizeBytes"] is None or len(blob) == expected["sizeBytes"]),
                "Committed/frozen native input mismatch: " + path)
        counts[path[len(PROJECT) + 1:].split("/")[0]] += 1
    members = archive_members(args.zip, args.rom.name)
    require(members[args.rom.name] == rom, "ZIP contains a substituted ROM")
    require(members["SHA256SUMS"] == (sha(rom) + "  " + args.rom.name + "\n").encode(),
            "ZIP ROM checksum text mismatch")
    native_header = header(rom)
    for path, member in (("LICENSE", "LICENSE"), (GAME + "/docs/ROM_NOTICES.txt", "ROM_NOTICES.txt")):
        require(members[member] == committed(commit, path), "ZIP notices differ from committed source")
    require(b"MIT License" in members["LICENSE"] and b"Hai Nghiem and contributors" in members["LICENSE"],
            "Project MIT notice missing")
    for text in ("Contains information licensed under the Open Government Licence – Toronto.",
                 "https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/",
                 "Chris Maltby", "Toxa", "Eric Provencher", "hUGEDriver", "GPLv2 with a linking exception"):
        require(text.encode() in members["ROM_NOTICES.txt"], "City/upstream attribution missing")
    guide = committed(commit, GAME + "/docs/LOADING.md").decode()
    links = []
    def resolve(match):
        parsed = urlsplit(match["target"].strip("<>"))
        if parsed.scheme or parsed.netloc or not parsed.path:
            return match.group(0)
        decoded = unquote(parsed.path)
        path = posixpath.normpath(posixpath.join(GAME + "/docs", decoded))
        require(not decoded.startswith("/") and path != ".." and not path.startswith("../"), "Unsafe guide link")
        committed(commit, path)
        links.append(path)
        url = URL + "/blob/" + commit + "/" + quote(path, safe="/")
        if parsed.query:
            url += "?" + parsed.query
        if parsed.fragment:
            url += "#" + quote(parsed.fragment, safe="-_.")
        return match["label"] + "(" + url + (match["title"] or "") + ")"
    require(members["LOADING.md"] == LINK.sub(resolve, guide).encode() and links,
            "ZIP guide differs from committed immutable-link guide")
    first = guide.split("\n\n", 2)[1]
    require(args.rom.name in first and sha(rom) in guide, "Guide does not select this exact ROM")
    info = unique_json(members["BUILDINFO.json"])
    require(isinstance(info, dict) and
            set(info) == {"game", "sourceCommit", "sourceUrl", "rom", "toolchain", "verification", "testingUrl"},
            "BUILDINFO schema mismatch")
    require(isinstance(info["toolchain"], dict) and isinstance(info["verification"], dict),
            "BUILDINFO toolchain/verification must be objects")
    require(info["game"] == "toronto-dispatch" and info["sourceCommit"] == commit and
            info["rom"] == {"filename": args.rom.name, "sizeBytes": len(rom), "sha256": sha(rom)} and
            info["sourceUrl"] == URL + "/tree/" + commit + "/" + GAME and
            info["testingUrl"] == URL + "/blob/" + commit + "/" + GAME + "/TESTING.md",
            "BUILDINFO source/ROM/public reference mismatch")
    build = committed(commit, GAME + "/docs/BUILD.md").decode()
    rows = {name.strip(): value.strip().replace("`", "") for name, value in
            re.findall(r"^\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*$", build, re.M)}
    tools = {"ModRetro Chromatic plugin", "GB Studio CLI", "GBVM engine", "GBDK", "Python / PyBoy", "Pillow", "Node.js"}
    require(set(info["toolchain"]) == tools and all(info["toolchain"][name] == rows[name] for name in tools),
            "Toolchain metadata differs from committed build pins")
    for key in ("cartridgeWrite", "physicalCartridgeBoot", "physicalSavePersistence", "twoHourGameplay"):
        require(info["verification"][key] == "not-verified", "Package must preserve independent verification boundaries")
    require("does not rebuild" in info["verification"]["romSourceCorrespondence"], "Source correspondence limit missing")
    for name, payload in members.items():
        if name != args.rom.name:
            require(PRIVATE.search(payload) is None, "Private host/state/credential pattern in ZIP text")
    baseline = []
    for name, (size, digest) in BASELINES.items():
        path = ROOT / PROJECT / "build" / name
        require(path.is_file() and not path.is_symlink() and path.stat().st_size == size and
                sha(path.read_bytes()) == digest, "Retained hardware baseline changed/missing")
        baseline.append({"filename": name, "sizeBytes": size, "sha256": digest})
    return {"result": "passed-scoped-package-source-audit", "sourceCommit": commit,
            "romSha256": sha(rom), "archiveSha256": sha(archive), "sourcePinsSha256": sha(pin_wire),
            "nativeInputs": len(pins), "nativeInputsByScope": dict(counts), "members": sorted(members),
            "header": native_header, "immutableGuideLinksChecked": len(links), "retainedBaselines": baseline,
            "limits": ["Complete assets/plugins/project/project.gbsproj scope, not every repository file.",
                       "Frozen pins supplied by caller; separate official build/native evidence must authenticate their origin.",
                       "No rebuild/source-to-ROM proof, native behavior, performance, cartridge write or physical acceptance.",
                       "Privacy allowlist/pattern review does not certify absence of every possible secret."]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--zip", type=Path, required=True)
    parser.add_argument("--expected-zip-sha256", required=True)
    parser.add_argument("--source-pins", type=Path, required=True)
    parser.add_argument("--expected-pins-sha256", required=True)
    args = parser.parse_args()
    try:
        result = audit(args)
    except (ValueError, KeyError, TypeError, OSError, zipfile.BadZipFile, zlib.error) as error:
        parser.exit(2, "Release audit failed: " + str(error) + "\n")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
