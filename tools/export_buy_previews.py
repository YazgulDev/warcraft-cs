"""Strip hands from private WCG2 caches for the original-CS-style buy panel."""
from __future__ import annotations
import struct
from pathlib import Path


def export_preview(source: Path, destination: Path, texture_names: list[str]) -> int:
    # Material names come from the player's studio model; never ship retail art or guessed silhouettes.
    data = source.read_bytes()
    magic, textures, meshes, sequences, bones = struct.unpack_from('<4s4I', data)
    if magic != b'WCG2' or textures > 128 or meshes > 256 or not 0 < bones <= 128:
        raise ValueError('Invalid preview source header')
    excluded = {i for i, name in enumerate(texture_names)
                if name.casefold().startswith('view_') or name.casefold() == 'silencer.bmp'}
    at = 20
    for _ in range(textures):
        width, height = struct.unpack_from('<2I', data, at)
        if not 0 < width <= 2048 or not 0 < height <= 2048:
            raise ValueError('Invalid preview texture size')
        at += 8 + width * height * 4
        if at > len(data):
            raise ValueError('Truncated preview textures')
    textures_end = at
    kept = []
    for _ in range(meshes):
        texture, count = struct.unpack_from('<2I', data, at)
        end = at + 8 + count * 24
        if texture >= textures or count > 100000 or count % 3 or end > len(data):
            raise ValueError('Invalid preview mesh')
        if texture not in excluded:
            kept.append(data[at:end])
        at = end
    if not kept:
        raise ValueError('No weapon geometry left after excluding hands')
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(struct.pack('<4s4I', magic, textures, len(kept), sequences, bones)
                            + data[20:textures_end] + b''.join(kept) + data[at:])
    return len(kept)
