"""Convert optional owned GoldSrc additive sprites into a bounded private RGBA cache."""
import struct
from pathlib import Path
from export_halflife_rpg import OwnedFiles


def decode(data: bytes) -> bytes:
    if len(data) < 42 or data[:4] != b'IDSP' or struct.unpack_from('<i', data, 4)[0] != 2:
        raise ValueError('Invalid GoldSrc sprite header')
    mode, count = struct.unpack_from('<i', data, 12)[0], struct.unpack_from('<i', data, 28)[0]
    colors = struct.unpack_from('<H', data, 40)[0]
    if mode != 1 or not 1 <= count <= 64 or colors != 256:
        raise ValueError('Expected a bounded additive palette sprite')
    palette = data[42:810]
    if len(palette) != 768:
        raise ValueError('Incomplete sprite palette')
    at, frames = 810, []
    for _ in range(count):
        kind, _, _, width, height = struct.unpack_from('<iiiii', data, at)
        at += 20
        if kind != 0 or not 1 <= width <= 512 or not 1 <= height <= 512:
            raise ValueError('Unsupported sprite frame')
        pixels = data[at:at + width * height]
        if len(pixels) != width * height:
            raise ValueError('Incomplete sprite pixels')
        at += len(pixels)
        # Additive GoldSrc art uses RGB intensity over black; preserve its original palette exactly.
        rgba = b''.join(palette[i*3:i*3+3] + b'\xff' for i in pixels)
        frames.append(struct.pack('<II', width, height) + rgba)
    if at != len(data):
        raise ValueError('Unexpected sprite trailer')
    result = b'WES1' + struct.pack('<I', count) + b''.join(frames)
    if len(result) > 4 * 1024 * 1024:
        raise ValueError('Sprite cache exceeds runtime limit')
    return result


def export(cstrike: Path, output: Path):
    source = OwnedFiles(cstrike.parent / 'valve')
    destination = output / 'effects'
    destination.mkdir(parents=True, exist_ok=True)
    for original, name in (('xbeam1.spr', 'egon_beam.wes'), ('XSpark1.spr', 'egon_impact.wes')):
        target = destination / name
        target.unlink(missing_ok=True)  # An absent/corrupt source must not retain a stale guest effect.
        try:
            data = source.read('sprites/' + original)
            if data is not None:
                target.write_bytes(decode(data))
        except (OSError, ValueError, struct.error):
            target.unlink(missing_ok=True)  # Optional effects never disable an otherwise complete weapon.
