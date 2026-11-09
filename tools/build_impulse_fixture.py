"""Build the original flat weapon-jump arena with matching terrain, pathing and shadows."""
from pathlib import Path
import argparse
import struct
from build_surf_map import build as empty_records
from build_surf_course import CELLS, terrain, info
from surf_course_layout import Pad
from map_archive import map_archive

ROOT = Path(__file__).resolve().parents[1]


def build():
    _, files = empty_records((55, 60, 65), 1600)
    # Replacing 64-cell terrain with the 128-cell arena also requires all fine-grid maps.
    # A stale SHD produces Warcraft's blocking "invalid shadow map file" dialog.
    side = CELLS * 4
    files.update({'war3map.w3e': terrain(0),
                  'war3map.wpm': b'MP3W' + struct.pack('<III', 0, side, side) + bytes(side * side),
                  'war3map.shd': bytes(side * side),
                  'war3map.w3i': info('Weapon impulse regression', 'Original isolated flat arena', Pad(0, 0, 0, 0)),
                  'war3map.j': (ROOT / 'tests/fixtures/ImpulseJumps.j').read_bytes()})
    return map_archive(files, 'Weapon impulse regression'), files


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(build()[0])
    print('Built original weapon impulse fixture:', args.output)
