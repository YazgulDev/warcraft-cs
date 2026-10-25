#!/usr/bin/env python3
"""Resolve JASS native function addresses from a Warcraft III Game.dll.

The classic Warcraft III engine registers every JASS native as a fixed four
instruction record emitted straight into .text:

    push   <signature string>      ; 68 xx xx xx xx
    mov    edx, <name string>      ; BA xx xx xx xx
    mov    ecx, <implementation>   ; B9 xx xx xx xx
    call   <registration helper>   ; E8 xx xx xx xx

`mov ecx` carries the address of the native implementation, so the record list
is a complete, self-describing name -> address map. The same shape exists in
1.26a and 1.27a, so a single scanner covers both builds and any future build
that keeps the pattern. A parallel copy of the list stores a shared `return 0`
stub instead of a real body; those records are reported separately because they
carry no usable address.

Usage:
    python tools/resolve_native_offsets.py <Game.dll> [--json out.json]

The output RVAs are relative to the module image base (0x6F000000 for every
classic Warcraft III build), matching the `base + offset` addressing used by
src/platform/NativeOffsets.inc.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys

IMAGE_BASE = 0x6F000000
# Every stub record in 1.27a points at this `xor eax,eax; ret` body.
ZERO_STUB_RVA = 0x70A0C0


class Section:
    __slots__ = ("name", "virtual_address", "virtual_size", "raw_offset", "raw_size")

    def __init__(self, name, virtual_address, virtual_size, raw_offset, raw_size):
        self.name = name
        self.virtual_address = virtual_address
        self.virtual_size = virtual_size
        self.raw_offset = raw_offset
        self.raw_size = raw_size

    def contains_rva(self, rva):
        return self.virtual_address <= rva < self.virtual_address + min(
            self.virtual_size, self.raw_size
        )


class Image:
    """Minimal 32-bit PE reader covering what the scanners need."""

    def __init__(self, data: bytes):
        self.data = data
        if data[:2] != b"MZ":
            raise ValueError("not a PE image")
        pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe_offset : pe_offset + 4] != b"PE\0\0":
            raise ValueError("missing PE signature")
        coff = pe_offset + 4
        section_count = struct.unpack_from("<H", data, coff + 2)[0]
        optional = coff + 20
        if struct.unpack_from("<H", data, optional)[0] != 0x10B:
            raise ValueError("expected a 32-bit PE image")
        self.image_base = struct.unpack_from("<I", data, optional + 28)[0]
        optional_size = struct.unpack_from("<H", data, coff + 16)[0]
        table = optional + optional_size
        self.sections = []
        for index in range(section_count):
            entry = table + index * 40
            name = data[entry : entry + 8].rstrip(b"\0").decode("latin-1")
            virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
                "<IIII", data, entry + 8
            )
            self.sections.append(
                Section(name, virtual_address, virtual_size, raw_offset, raw_size)
            )

    def rva_to_offset(self, rva):
        for section in self.sections:
            if section.contains_rva(rva):
                return section.raw_offset + (rva - section.virtual_address)
        return None

    def offset_to_rva(self, offset):
        for section in self.sections:
            if section.raw_offset <= offset < section.raw_offset + section.raw_size:
                return section.virtual_address + (offset - section.raw_offset)
        return None

    def section_name(self, rva):
        for section in self.sections:
            if section.contains_rva(rva):
                return section.name
        return None

    def cstring(self, rva, limit=90):
        offset = self.rva_to_offset(rva)
        if offset is None:
            return None
        if not (0x94E000 <= rva < 0xB59000):  # .rdata string pool only
            return None
        end = self.data.find(b"\0", offset, offset + limit)
        if end <= offset or end - offset > limit:
            return None
        raw = self.data[offset:end]
        if any(byte < 0x20 or byte > 0x7E for byte in raw):
            return None
        return raw.decode("ascii")

    def file_version(self):
        """Return (a, b, c, d) from the binary's version resource, if present."""
        key = "F\0i\0l\0e\0V\0e\0r\0s\0i\0o\0n\0".encode("ascii")
        start = self.data.find(key)
        if start < 0:
            return None
        cursor = start + len(key)
        while cursor < len(self.data) and self.data[cursor] == 0:
            cursor += 1
        # Value length and type precede the UTF-16 payload in the VS_VERSIONINFO block.
        if cursor + 6 > len(self.data):
            return None
        length, _value_type = struct.unpack_from("<HH", self.data, cursor)
        cursor += 6
        text = self.data[cursor : cursor + length].decode("utf-16-le", "replace")
        text = text.split("\0")[0]
        parts = text.split(".")
        if len(parts) != 4 or not all(part.isdigit() for part in parts):
            return None
        return tuple(int(part) for part in parts)


def scan_registrations(image: Image):
    """Return (real, stub) lists of registration records found in .text."""
    data = image.data
    real, stub = [], []
    cursor = 0
    end = len(data) - 21
    while cursor < end:
        if data[cursor] == 0x68 and data[cursor + 5] == 0xBA:
            signature = image.cstring(
                struct.unpack_from("<I", data, cursor + 1)[0] - image.image_base
            )
            name = image.cstring(
                struct.unpack_from("<I", data, cursor + 6)[0] - image.image_base
            )
            if signature and name and signature.startswith("(") and name[:1].isalpha():
                implementation = struct.unpack_from("<I", data, cursor + 11)[0]
                if data[cursor + 10] == 0xB9 and data[cursor + 15] == 0xE8:
                    rva = image.offset_to_rva(cursor)
                    record = {
                        "name": name,
                        "signature": signature,
                        "rva": implementation - image.image_base,
                        "site": rva,
                        "section": image.section_name(rva),
                    }
                    if record["rva"] == ZERO_STUB_RVA:
                        stub.append(record)
                    else:
                        real.append(record)
                    cursor += 1
                    continue
        cursor += 1
    return real, stub


def find_bytes(image: Image, pattern: bytes, start=0x1000, end=None):
    """Return every .text RVA where `pattern` occurs."""
    text = next((s for s in image.sections if s.name == ".text"), None)
    if text is None:
        return []
    end = end if end is not None else text.virtual_address + text.virtual_size
    first = text.raw_offset + (start - text.virtual_address)
    last = text.raw_offset + (end - text.virtual_address)
    hits, cursor = [], first
    while True:
        found = image.data.find(pattern, cursor, last)
        if found < 0:
            return hits
        hits.append(text.virtual_address + (found - text.raw_offset))
        cursor = found + 1


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game_dll", help="path to the Game.dll to inspect")
    parser.add_argument("--json", help="write the resolver output to this file")
    args = parser.parse_args(argv)

    with open(args.game_dll, "rb") as handle:
        image = Image(handle.read())

    real, stub = scan_registrations(image)
    version = image.file_version()
    print(f"image base      : {image.image_base:#x}")
    print(f"file version    : {'.'.join(map(str, version)) if version else 'unknown'}")
    print(f"native records  : {len(real)} implemented, {len(stub)} stub")
    if real:
        names = [record["name"] for record in real]
        print(f"distinct natives: {len(set(names))}")
    duplicates = sorted({n for n in (r['name'] for r in real) if
                         sum(1 for r in real if r['name'] == n) > 1})
    if duplicates:
        print(f"duplicate names : {len(duplicates)}")

    if args.json:
        payload = {
            "file_version": version,
            "image_base": image.image_base,
            "natives": {r["name"]: r["rva"] for r in real},
            "signatures": {r["name"]: r["signature"] for r in real},
            "stubbed": sorted({r["name"] for r in stub}),
        }
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump(payload, handle, indent=2, sort_keys=True)
        print(f"written         : {args.json}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
