"""Convert only the owner's loose CS 1.6 sky textures into private RGBA face caches."""
import argparse
import json
import struct
from pathlib import Path

FACES = ("rt", "lf", "bk", "ft", "up", "dn")


def read_tga(path):
    data = path.read_bytes()
    if len(data) < 18:
        raise ValueError("Truncated TGA")
    ident, palette, kind = data[:3]
    width, height, depth, flags = struct.unpack_from("<HHBB", data, 12)
    if palette or kind not in (2, 10) or depth not in (24, 32) or not (0 < width <= 2048 and 0 < height <= 2048):
        raise ValueError("Unsupported sky TGA")
    # Decode raw/RLE true-color TGA locally; no Pillow or new client dependency is required.
    stride = depth // 8
    cursor = 18 + ident
    pixels = bytearray()
    expected = width * height * stride
    if kind == 2:
        pixels.extend(data[cursor:cursor + expected])
    else:
        while len(pixels) < expected:
            if cursor >= len(data):
                raise ValueError("Truncated TGA packet")
            packet = data[cursor]
            cursor += 1
            count = (packet & 127) + 1
            if packet & 128:
                pixel = data[cursor:cursor + stride]
                cursor += stride
                pixels.extend(pixel * count)
            else:
                size = count * stride
                pixels.extend(data[cursor:cursor + size])
                cursor += size
            if len(pixels) > expected:
                raise ValueError("TGA packet overflow")
    if len(pixels) != expected:
        raise ValueError("Truncated TGA pixels")
    rgba = bytearray()
    # Normalize to top-left rows; the cube renderer consumes a consistent face orientation.
    for y in range(height):
        source_y = y if flags & 32 else height - 1 - y
        for x in range(width):
            source_x = width - 1 - x if flags & 16 else x
            at = (source_y * width + source_x) * stride
            b, g, r = pixels[at:at + 3]
            rgba.extend((r, g, b, 255))
    return width, height, bytes(rgba)


def export(cstrike, output):
    source = cstrike / "gfx" / "env"
    output.mkdir(parents=True, exist_ok=True)
    manifest = []
    # Optional sky art must never make an otherwise valid gun/sound install fail.
    for top in sorted(source.glob("*up.tga")):
        name = top.stem[:-2]
        if not name or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" for c in name):
            continue
        try:
            faces = [read_tga(source / (name + face + ".tga")) for face in FACES]
            if len({(w, h) for w, h, _ in faces}) != 1 or faces[0][0] != faces[0][1]:
                raise ValueError("Sky faces must be equally sized squares")
            width, height, _ = faces[0]
            target = output / (name + ".wcs")
            target.write_bytes(struct.pack("<4sII", b"WCS1", width, height) + b"".join(p[2] for p in faces))
            manifest.append({"sky": name, "size": width, "cache_bytes": target.stat().st_size})
        except (OSError, ValueError) as error:
            print("Skipped sky", name, str(error))
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cstrike", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(export(args.cstrike, args.output), indent=2))
