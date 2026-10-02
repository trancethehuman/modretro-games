#!/usr/bin/env python3
"""Package one inspected native ROM with committed loading and licence notices.

Run after committing the matching game source. This verifies identities and
packages files; it does not build, playtest, write a cartridge or prove that the
supplied ROM was built from the declared commit.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import posixpath
import re
import subprocess
import tempfile
from urllib.parse import quote, unquote, urlsplit
import zipfile

REPOSITORY = Path(__file__).resolve().parents[1]
SOURCE_URL = "https://github.com/trancethehuman/modretro-games"
SAFE_FILENAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9._-]*")
MARKDOWN_LINK = re.compile(
    r'(?P<label>!?\[[^\]\n]*\])\((?P<target><[^>\n]+>|[^\s)]+)'
    r'(?P<title>\s+"[^"\n]*")?\)'
)


def git(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", "-C", str(REPOSITORY), *args],
        capture_output=True,
        text=True,
        check=False,
    )


def committed_text(commit: str, path: str) -> str:
    result = git("show", f"{commit}:{path}")
    if result.returncode:
        raise ValueError(f"Required file is missing from the source commit: {path}")
    return result.stdout


def source_revision(value: str) -> str:
    if not re.fullmatch(r"[0-9a-fA-F]{40}", value):
        raise ValueError("--source-commit must be a full 40-character Git commit SHA")
    commit = value.lower()
    result = git("cat-file", "-t", commit)
    if result.returncode or result.stdout.strip() != "commit":
        raise ValueError("The source commit does not exist in this repository")
    return commit


def public_loading_links(markdown: str, commit: str, document_path: str) -> str:
    """Resolve relative links against the original document, never ZIP layout."""
    def replace(match: re.Match[str]) -> str:
        target = match["target"].strip("<>")
        parsed = urlsplit(target)
        if parsed.scheme or parsed.netloc or not parsed.path:
            return match.group(0)
        if parsed.path.startswith("/"):
            raise ValueError("Loading instructions contain an absolute local link")
        resolved = posixpath.normpath(
            posixpath.join(posixpath.dirname(document_path), unquote(parsed.path))
        )
        if resolved == ".." or resolved.startswith("../"):
            raise ValueError("A loading link escapes the source repository")
        if git("cat-file", "-e", f"{commit}:{resolved}").returncode:
            raise ValueError(f"Loading link is missing from the source commit: {resolved}")
        url = f"{SOURCE_URL}/blob/{commit}/{quote(resolved, safe='/')}"
        if parsed.query:
            url += "?" + parsed.query
        if parsed.fragment:
            url += "#" + quote(parsed.fragment, safe="-_.")
        return f'{match["label"]}({url}{match["title"] or ""})'

    return MARKDOWN_LINK.sub(replace, markdown)


def pinned_toolchain(build_document: str) -> dict[str, str]:
    rows = dict(
        (name.strip(), identity.strip().replace("`", ""))
        for name, identity in re.findall(
            r"^\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*$",
            build_document,
            flags=re.MULTILINE,
        )
    )
    required = (
        "ModRetro Chromatic plugin", "GB Studio CLI", "GBVM engine", "GBDK",
        "Python / PyBoy", "Pillow", "Node.js",
    )
    if any(name not in rows for name in required):
        raise ValueError("Committed BUILD.md is missing tested toolchain identities")
    return {name: rows[name] for name in required}


def output_path(value: Path) -> Path:
    if not SAFE_FILENAME.fullmatch(value.name) or value.suffix != ".zip":
        raise ValueError("--output must have a safe filename ending in .zip")
    if value.is_symlink():
        raise ValueError("--output cannot be a symbolic link")
    result = value.resolve()
    try:
        relative = result.relative_to(REPOSITORY)
    except ValueError:
        raise ValueError("--output must be inside this repository's ignored build tree") from None
    if "build" not in relative.parts[:-1]:
        raise ValueError("--output must be inside a build/ directory")
    if git("check-ignore", "--quiet", str(relative)).returncode:
        raise ValueError("--output is not ignored by Git; use an ignored build/ tree")
    if result.exists() and not result.is_file():
        raise ValueError("--output already names something other than a regular file")
    return result


def bundle(args: argparse.Namespace) -> dict[str, object]:
    if not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", args.game):
        raise ValueError("--game must be a lowercase game slug")
    if not re.fullmatch(r"[0-9a-fA-F]{64}", args.expected_sha256):
        raise ValueError("--expected-sha256 must be the full inspected SHA-256")
    expected = args.expected_sha256.lower()
    commit = source_revision(args.source_commit)
    output = output_path(args.output)
    rom = args.rom
    if rom.is_symlink() or not rom.is_file():
        raise ValueError("--rom must select an existing regular file, not a symlink")
    if not SAFE_FILENAME.fullmatch(rom.name) or rom.suffix.lower() not in (".gb", ".gbc"):
        raise ValueError("--rom must have a safe native .gb or .gbc filename")
    rom = rom.resolve()
    game_path = f"games/{args.game}"
    try:
        rom.relative_to(REPOSITORY / game_path / "project" / "build")
    except ValueError:
        raise ValueError("--rom must be the selected game's native project/build ROM") from None
    data = rom.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != expected:
        raise ValueError("ROM SHA-256 does not match --expected-sha256; nothing was packaged")
    if len(data) < 0x150 or len(data) > 8 * 1024 * 1024:
        raise ValueError("ROM size is outside the native cartridge packaging range")
    header_checksum = 0
    for byte in data[0x134:0x14D]:
        header_checksum = (header_checksum - byte - 1) & 0xFF
    if header_checksum != data[0x14D] or not data[0x143] & 0x80:
        raise ValueError("ROM must have a valid Game Boy Color header")

    loading_path = f"{game_path}/docs/LOADING.md"
    loading = public_loading_links(committed_text(commit, loading_path), commit, loading_path)
    notices = committed_text(commit, f"{game_path}/docs/ROM_NOTICES.txt")
    licence = committed_text(commit, "LICENSE")
    build_document = committed_text(commit, f"{game_path}/docs/BUILD.md")
    build_info = {
        "game": args.game,
        "sourceCommit": commit,
        "sourceUrl": f"{SOURCE_URL}/tree/{commit}/{game_path}",
        "rom": {"filename": rom.name, "sizeBytes": len(data), "sha256": digest},
        "toolchain": pinned_toolchain(build_document),
        "verification": {
            "packaging": "Exact supplied SHA-256 and valid CGB header checked",
            "emulator": "See the source commit's TESTING.md for this exact ROM digest",
            "romSourceCorrespondence": "Declared by the caller; packaging does not rebuild the ROM",
            "cartridgeWrite": "not-verified",
            "physicalCartridgeBoot": "not-verified",
            "physicalSavePersistence": "not-verified",
            "twoHourGameplay": "not-verified",
        },
        "testingUrl": f"{SOURCE_URL}/blob/{commit}/{game_path}/TESTING.md",
    }
    files = {
        rom.name: data,
        "SHA256SUMS": f"{digest}  {rom.name}\n".encode(),
        "ROM_NOTICES.txt": notices.encode(),
        "LOADING.md": loading.encode(),
        "LICENSE": licence.encode(),
        "BUILDINFO.json": (json.dumps(build_info, indent=2, sort_keys=True) + "\n").encode(),
    }
    # Inspect every included text payload, including extracted toolchain metadata.
    for name, payload in files.items():
        if name == rom.name:
            continue
        if re.search(rb"/Users/|/home/|https?://(?:127\.0\.0\.1|localhost)(?::|/)", payload):
            raise ValueError("Release text contains a private host path or localhost URL")
    # These are the entire allowlist: never walk build directories or copy logs/saves.
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix=".package-", suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for name in sorted(files):
                info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                archive.writestr(info, files[name])
        with zipfile.ZipFile(temporary) as archive:
            if archive.namelist() != sorted(files) or archive.testzip() is not None:
                raise ValueError("Archive verification failed")
            if hashlib.sha256(archive.read(rom.name)).hexdigest() != digest:
                raise ValueError("Archived ROM checksum verification failed")
        os.replace(temporary, output)
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()
    return {
        "archive": str(output.relative_to(REPOSITORY)),
        "archiveSha256": hashlib.sha256(output.read_bytes()).hexdigest(),
        "rom": build_info["rom"],
        "sourceCommit": commit,
        "members": sorted(files),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--game", default="toronto-dispatch")
    args = parser.parse_args()
    try:
        result = bundle(args)
    except (ValueError, OSError) as error:
        parser.exit(2, f"Packaging failed: {error}\n")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
