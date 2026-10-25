#!/usr/bin/env python3
"""Generate src/platform/NativeOffsets<version>.inc from a resolved Game.dll.

Reads the 1.26a reference table, resolves every declared native by *name* in a
resolved-native JSON produced by tools/resolve_native_offsets.py, cross-checks
each entry against the JASS signature the engine itself stores, and writes an
offset include in the same shape as the reference.

The signature check is the safety net that makes this a port rather than a
guess: a wrong name match cannot also produce a matching parameter list, so any
mismatch is reported and generation fails before replacing a complete declaration table.

Usage:
    python tools/generate_native_offsets.py \
        --reference src/platform/NativeOffsets.inc \
        --natives .local/game-1.27.0.52240.json \
        --output src/platform/NativeOffsets127a.inc \
        --version "1.27.0.52240 (f7ba2a3)"
"""

from __future__ import annotations

import argparse
import json
import re
import sys

NATIVE_PATTERN = re.compile(
    r"NATIVE\(\s*([A-Za-z_][\w\s\*]*?)\s*,\s*(\w+)\s*,\s*\((.*?)\)\s*,\s*(0x[0-9A-Fa-f]+)\)"
)

# Engine internals the runtime calls directly rather than through the JASS native
# table. Each value was located by following a *structural* fact - which JASS
# wrapper calls it, which object member it touches, or where a sibling native
# points - and never by guessing a nearby address. Entries that could not be
# confirmed that way are deliberately absent so Rva() returns 0 and the owning
# feature disables itself instead of calling into an unverified location.
INTERNAL_ADDRESSES = {
    0x3BDCB0: (0x1D1550, "CUnit handle resolver; called by 326 unit natives with ecx=handle"),
    0x3BE010: (0x1D17D0, "CWidget handle resolver; GetWidgetX/Y, AddSpecialEffect, GroupRemoveUnit"),
    0x300710: (0x34F3A0, "UI accessor GetUI(index,serial); call sites use (0,0) and (1,0)"),
    0x2F5ED0: (0x34DA20, "camera getter; byte-identical `mov eax,[ecx+254h]; ret`"),
    0x4C4630: (0x0516F0, "CString getter; byte-identical `mov eax,[ecx+8]` node walk"),
    0x430C80: (0x2651D0, "object to JASS handle; CreateUnit/CreateItem/CreateDestructable/FirstOfGroup"),
    0x390590: (0x36D620, "world sky setter; touches [this+354h], SetSkyModel passes [ui+3BCh]"),
    0x3B4C62: (0x1E1884, "camera bound minX getter inside GetCameraBoundMinX (+12h to +14h)"),
    0x3B4740: (0x1F19A0, "PanCameraToTimed"),
    0x3B47D0: (0x1F1A00, "PanCameraToTimedWithZ"),
    0x3CD7B0: (0x1F41B0, "SetCameraOrientController"),
    0x3CD900: (0x1DAD30, "CameraSetupApply"),
    0x3CD930: (0x1DADC0, "CameraSetupApplyWithZ"),
    0x3CD960: (0x1DAD60, "CameraSetupApplyForceDuration"),
    0x3CD990: (0x1DAD90, "CameraSetupApplyForceDurationWithZ"),
    0x3C8DA0: (0x1E9450, "IssuePointOrder"),
    0x3C8E20: (0x1E96A0, "IssueTargetOrder"),
    0x3BAD70: (0x1F18D0, "OrderId"),
    0x3BC4D0: (0x1F1AD0, "PauseGame"),
    # Frame-rate demand clear. Both builds execute the identical instruction shape
    # `mov eax,[<reg>+44h]; test al,4; jz short; and eax,-5; mov [<reg>+44h],eax`
    # inside the per-frame world update, and the only byte differences are the
    # register encoding (1.26a esi, 1.27a edi) and the jz displacement. Flipping the
    # final `-5` operand to -1 leaves the demand bit set, which is the unlock.
    0x62D7F2: (0x5FCF2, "render-demand clear; and eax,-5 at [this+44h] (1.26a: esi / jz +14h, 1.27a: edi / jz +11h)"),
    # Unit ability lookup and the list-node resolver it calls. Located from GetUnitAbilityLevel
    # (RVA 0x1E5EF0, already mapped): its body is `push 1;push 1;push 1;push 0;push id;
    # mov ecx,CUnit*; call <lookup>` with `ret 14h`. The callee dispatches on the id (the same
    # engine constant 0x414E6275 the 1.26a build uses) and then walks the [unit+1DCh] ability
    # list with an `and eax,[ecx]; cmp eax,-1` sentinel - structurally the same function the
    # 1.26a table calls 0x787D0. The {generation,index} validator invoked from inside that walk
    # with ecx=link is the sibling 1.26a calls at 0x4786B0.
    0x787D0: (0x46F440, "CUnit ability lookup behind GetUnitAbilityLevel; dispatches id then walks [unit+1DCh]"),
    0x4786B0: (0x44150, "ability list {generation,index} resolver; called from the ability lookup with ecx=link"),
}

# Accepted JASS signature letters for each C declaration type. The reference table
# deliberately declares enum handles (playerstate, unitstate, camerafield, itemtype,
# pathingtype, ...) and JASS string/code wrappers as `int` or `Handle`, because the
# runtime treats them as opaque values. Family matching keeps the check meaningful
# (arity plus value class) without rejecting those intentional aliases.
TYPE_LETTERS = {
    "void": "V",
    "BOOL": "B",
    "int": "IH",
    "uint32_t": "IH",  # Explicit native handles used by selection/build-order declarations.
    "Bits": "R",
    "float*": "R",
    "Handle": "HSC",
}


def expected_shape(return_type: str, arguments: str):
    """Return the (parameter letters, return letter) families a signature must fit."""
    params = [part.strip() for part in arguments.split(",") if part.strip()]
    letters = []
    for param in params:
        if param not in TYPE_LETTERS:
            return None
        letters.append(TYPE_LETTERS[param])
    if return_type not in TYPE_LETTERS:
        return None
    return letters, TYPE_LETTERS[return_type]


def jass_shape(signature: str):
    """Split a JASS signature such as (Hunit;IRR)Hitem; into letters."""
    match = re.match(r"^\((.*)\)(.)", signature)
    if not match:
        return None
    body, result = match.group(1), match.group(2)
    letters = []
    index = 0
    while index < len(body):
        letter = body[index]
        if letter in "VBIHR":
            letters.append(letter)
            index += 1
            if letter == "H":
                # Handles carry ";terminated" type names; the parameters follow after it.
                end = body.find(";", index)
                index = len(body) if end < 0 else end + 1
        elif letter in "SC":
            letters.append(letter)
            index += 1
        else:
            return None
    return "".join(letters), result


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", required=True)
    parser.add_argument("--natives", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--build-table", help="also write the GameBuild translation include")
    parser.add_argument("--image-base", default="0x6F000000")
    args = parser.parse_args(argv)

    with open(args.natives, encoding="utf-8") as handle:
        resolved = json.load(handle)
    available = resolved["natives"]
    signatures = resolved["signatures"]
    image_base = int(args.image_base, 16)

    with open(args.reference, encoding="utf-8") as handle:
        lines = handle.read().splitlines()

    output, accepted, rejected = [], [], []
    output.append(f"// Game.dll {args.version} native entry points.")
    output.append("// Generated by tools/generate_native_offsets.py; every entry was matched by name")
    output.append("// and re-checked against the JASS signature the engine itself stores, so a stale")
    output.append("// or colliding name cannot be accepted silently. Offsets are RVAs from the module")
    output.append("// image base, matching the `base + offset` binding used by WarcraftApi.cpp.")
    output.append("// A zero offset means this build has no verified entry; Bind() refuses to guess.")

    for line in lines:
        match = NATIVE_PATTERN.search(line)
        if not match:
            output.append(line)
            continue
        return_type, name, arguments, legacy_rva = match.groups()
        shape = expected_shape(return_type, arguments)
        if name not in available:
            rejected.append((name, "absent from this build"))
            output.append(f"// {name}: absent from this build")
            continue
        observed = jass_shape(signatures.get(name, ""))
        if not shape or not observed:
            # Missing type/signature evidence must not produce an apparently valid native pointer.
            rejected.append((name, "missing or unsupported declaration/signature"))
            continue
        if shape and observed:
            families, result_family = shape
            letters, result = observed
            fits = (
                len(families) == len(letters)
                and all(letter in family for letter, family in zip(letters, families))
                and result in result_family
            )
            if not fits:
                rejected.append(
                    (name, f"signature {signatures[name]} does not fit ({arguments}){return_type}")
                )
                output.append(f"// {name}: signature {signatures[name]} rejected")
                continue
        offset = available[name] + image_base - image_base
        accepted.append((name, legacy_rva, offset, signatures.get(name, "")))
        output.append(
            f"NATIVE({return_type}, {name}, ({arguments}), 0x{offset:X})"
            f"  // was {legacy_rva}, {signatures.get(name, '')}"
        )

    if rejected:
        # A partial declaration table would leave required native pointers null or bound to legacy entries.
        for name, reason in rejected:
            print(f"Rejected {name}: {reason}", file=sys.stderr)
        return 1

    with open(args.output, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(output) + "\n")

    if args.build_table:
        rows = [
            f"// Game.dll {args.version} address translation table.",
            "// Generated by tools/generate_native_offsets.py; do not edit by hand.",
            "//",
            "// BUILD_ADDRESS(1.26a RVA, this build's RVA). GameBuild.cpp resolves legacy",
            "// offsets through this list, so a missing row intentionally yields 0 and makes",
            "// the owning feature stand down rather than call an unverified address.",
            "",
            "// --- Engine internals called directly by the runtime ---",
        ]
        for legacy in sorted(INTERNAL_ADDRESSES):
            translated, comment = INTERNAL_ADDRESSES[legacy]
            rows.append(f"BUILD_ADDRESS(0x{legacy:X}, 0x{translated:X})  // {comment}")
        # Public wrappers invoked by helper methods rather than NATIVE declarations are resolved by name too.
        for legacy, name in ((0x3C61A0, "SetUnitAnimation"), (0x3C61F0, "SetUnitAnimationByIndex"), (0x3C6300, "SetUnitTimeScale")):
            rows.append(f"BUILD_ADDRESS(0x{legacy:X}, 0x{available[name]:X})  // Registered {name} {signatures[name]}")
        rows.append("")
        rows.append("// --- JASS native entry points also referenced by legacy RVA ---")
        for name, legacy_rva, offset, signature in sorted(accepted, key=lambda r: int(r[1], 16)):
            # A zero legacy RVA is a 1.27a-only native: the reference build has no verified
            # address for it, so there is nothing for the translation table to resolve.
            if int(legacy_rva, 16) == 0:
                continue
            rows.append(
                f"BUILD_ADDRESS({legacy_rva}, 0x{offset:X})  // {name} {signature}"
            )
        with open(args.build_table, "w", encoding="utf-8", newline="\n") as handle:
            handle.write("\n".join(rows) + "\n")
        print(f"build    : {args.build_table} ({len(INTERNAL_ADDRESSES)} internals, "
              f"{len(accepted)} natives)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
