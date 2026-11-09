"""Reject mismatched shadow/pathing dimensions before launching the authored arena."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from build_impulse_fixture import build


class NativeFixtureMapTests(unittest.TestCase):
    def test_fine_grids_match_actual_terrain_dimensions(self):
        raw, files = build()
        self.assertEqual(raw[512:516], b'MPQ\x1a')
        terrain = files['war3map.w3e']
        tile_count = struct.unpack_from('<I', terrain, 13)[0]
        at = 17 + tile_count * 4
        cliff_count = struct.unpack_from('<I', terrain, at)[0]
        width, height = struct.unpack_from('<II', terrain, at + 4 + cliff_count * 4)
        side_x, side_y = (width - 1) * 4, (height - 1) * 4
        # The engine consumes one shadow/pathing byte per fine cell, with no SHD header.
        self.assertEqual(len(files['war3map.shd']), side_x * side_y)
        pathing = files['war3map.wpm']
        self.assertEqual(struct.unpack_from('<II', pathing, 8), (side_x, side_y))
        self.assertEqual(len(pathing) - 16, side_x * side_y)
        self.assertIn(b'function ImpulseChat', files['war3map.j'])
        self.assertFalse(any(name.endswith(('.dll', '.mix', '.mpq')) for name in files))


if __name__ == '__main__':
    unittest.main()
