"""Create a complete original single-player surfing map using only Python's standard library."""
from __future__ import annotations
import argparse
import math
from pathlib import Path
import struct
from map_archive import map_archive

ROOT = Path(__file__).resolve().parents[1]
CELLS, ORIGIN = 64, -4096
LANES = (-2304, 0, 2304)
START_X, RAMP_X, FINISH_X = -2688, -1536, 1024
BASE = 100


def text(value: str) -> bytes:
    return value.encode('utf-8') + b'\0'


def height_at(x: float, y: float, angles: tuple[float, ...], rise: float) -> float:
    # Each wide lane has a flat run-up, a continuous plane and a flat landing. Shoulders
    # fade into the empty safety area; the map trigger returns players who leave their lane.
    for index, center in enumerate(LANES):
        distance = abs(y - center)
        if distance <= 896:
            shoulder = max(0.0, min(1.0, (896 - distance) / 256))
            altitude = max(0.0, min(rise, rise - max(0, x - RAMP_X) * math.tan(math.radians(angles[index]))))
            return BASE + altitude * shoulder
    return BASE


def terrain(angles: tuple[float, ...], rise: float) -> bytes:
    # Classic W3E v11 references installed stock tiles by identifier, never embeds their art.
    tiles = (b'Ldrt', b'Ldro', b'Lgrs', b'Ysqd')
    data = bytearray(b'W3E!' + struct.pack('<IcII', 11, b'L', 1, len(tiles),))
    data += b''.join(tiles) + struct.pack('<I', 2) + b'CLdiCLgr'
    data += struct.pack('<IIff', CELLS + 1, CELLS + 1, ORIGIN, ORIGIN)
    for row in range(CELLS + 1):
        for column in range(CELLS + 1):
            x, y = ORIGIN + column * 128, ORIGIN + row * 128
            elevation = round(8192 + height_at(x, y, angles, rise) * 4)
            if not 0 <= elevation < 16384:
                raise ValueError('Terrain height exceeds the classic representation')
            lane = next((i for i, cy in enumerate(LANES) if abs(y-cy) <= 640), None)
            tile = 0 if lane is None else 2 if x <= RAMP_X else 3 if x >= 256 else (column + row) % 2
            data += struct.pack('<HHBBB', elevation, 0, tile, 0, 2)
    return bytes(data)


def info() -> bytes:
    bounds = (-3712., -3712., 3712., 3712., -3712., 3712., 3712., -3712.)
    data = struct.pack('<III', 18, 1, 6052)
    data += text('Warcraft CS - Surf Practice') + text('Yazgul / Codex')
    data += text('Three surfing lanes. Requires the Warcraft CS surfing build. F6: FPS. -help: controls.') + text('1 player')
    data += struct.pack('<8f4i3I', *bounds, 3, 3, 3, 3, CELLS-6, CELLS-6, 0x60) + b'L'
    data += struct.pack('<i', -1) + text('F6: first person. -surf 1/2/3: choose lane. -reset: restart. -auto: automatic return.')
    data += text('Surf Practice') + text('Warcraft CS') + struct.pack('<i', -1) + text('') * 3
    data += struct.pack('<5I', 1, 0, 1, 1, 1) + text('Surfer')
    data += struct.pack('<ffII', START_X, LANES[0], 0, 0)
    data += struct.pack('<III', 1, 1, 1) + text('Surf practice')
    return data + struct.pack('<III', 0, 0, 0)


def preview(angles: tuple[float, ...], rise: float) -> bytes:
    # An original procedural TGA supplies a useful map-browser overview and native minimap.
    size = 256
    image = bytearray(struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, size, size, 24, 0x20))
    colors = ((235, 163, 55), (75, 165, 235), (205, 95, 220))
    for row in range(size):
        for column in range(size):
            x, y = ORIGIN + column * 8192 / size, -ORIGIN - row * 8192 / size
            color = (48, 55, 47)
            for lane, center in enumerate(LANES):
                if abs(y-center) <= 640:
                    color = (70, 165, 85) if x <= RAMP_X else (160, 168, 165) if x > 256 else colors[lane]
                    if x < -3520 or x > FINISH_X:
                        color = tuple(int(c*.65) for c in color)
            image += bytes(reversed(color))
    return bytes(image)


def build(angles: tuple[float, ...], rise: float) -> tuple[bytes, dict[str, bytes]]:
    if len(angles) != 3 or any(not math.isfinite(a) or not 48 <= a <= 75 for a in angles):
        raise ValueError('Exactly three finite angles between 48 and 75 degrees are required')
    if not math.isfinite(rise) or not 512 <= rise <= 1800:
        raise ValueError('Rise must be between 512 and 1800 Warcraft units')
    script = (ROOT/'tools/maps/SurfPractice.j').read_text(encoding='utf-8')
    setup = '\n'.join(f'    set laneY[{i+1}]={y}.\n    set laneAngle[{i+1}]={a:g}' for i, (y, a) in enumerate(zip(LANES, angles)))
    script = script.replace('@LANE_SETUP@', setup)
    path_size = CELLS * 4
    files = {'war3map.w3i': info(), 'war3map.w3e': terrain(angles, rise),
             'war3map.wpm': b'MP3W' + struct.pack('<III', 0, path_size, path_size) + bytes(path_size*path_size),
             'war3map.j': script.encode('utf-8'),
             'war3map.doo': b'W3do' + struct.pack('<5I', 8, 11, 0, 0, 0),
             'war3mapUnits.doo': b'W3do' + struct.pack('<3I', 8, 11, 0),
             'war3map.w3r': struct.pack('<II', 5, 0), 'war3map.w3c': struct.pack('<II', 0, 0),
             'war3map.w3s': struct.pack('<II', 1, 0),
             'war3map.mmp': struct.pack('<II', 0, 0),
             'war3map.shd': bytes(path_size*path_size),
             'war3mapMap.tga': preview(angles, rise), 'war3mapPreview.tga': preview(angles, rise)}
    return map_archive(files, 'Warcraft CS - Surf Practice'), files


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'.local/surf-practice/WarcraftCS-Surf-Practice.w3m')
    parser.add_argument('--angles', type=float, nargs=3, default=(55, 60, 65))
    parser.add_argument('--rise', type=float, default=1600)
    parser.add_argument('--script-output', type=Path, help='Optional authored JASS output for an installed syntax checker')
    args = parser.parse_args()
    data, files = build(tuple(args.angles), args.rise)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    if args.script_output:
        args.script_output.write_bytes(files['war3map.j'])
    print(f'Built {args.output}: {len(data)} bytes, {len(files)} original files; no retail map/assets read.')


if __name__ == '__main__':
    main()
