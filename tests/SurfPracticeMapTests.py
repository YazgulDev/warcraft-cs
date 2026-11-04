"""Check authored course limits before loading a classic Warcraft map."""
import math
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from build_surf_map import BASE, CELLS, LANES, ORIGIN, RAMP_X, START_X, build


class SurfPracticeMapTests(unittest.TestCase):
    def test_native_height_and_spawn_clearance(self):
        # Catch height-bit wrapping and a spawn placed above a cliff/shoulder, not just a valid MPQ.
        raw, files = build((55, 60, 65), 1600)
        self.assertEqual(raw[:4], b'HM3W')
        self.assertEqual(raw[512:516], b'MPQ\x1a')
        data = files['war3map.w3e']
        tiles = struct.unpack_from('<I', data, 13)[0]
        at = 17 + tiles*4
        cliffs = struct.unpack_from('<I', data, at)[0]
        at += 4 + cliffs*4
        width, height, ox, oy = struct.unpack_from('<IIff', data, at)
        at += 16
        self.assertEqual((width, height, ox, oy), (CELLS+1, CELLS+1, ORIGIN, ORIGIN))
        self.assertEqual(len(data)-at, width*height*7)
        elevations = [struct.unpack_from('<H', data, at+n*7)[0] for n in range(width*height)]
        self.assertGreaterEqual(min(elevations), 8192 + BASE*4)
        self.assertLess(max(elevations), 16384)
        for lane, angle in zip(LANES, (55, 60, 65)):
            row = int((lane-ORIGIN)/128)
            spawn = int((START_X-ORIGIN)/128)
            for r in range(row-1, row+2):
                for c in range(spawn-1, spawn+2):
                    self.assertEqual(elevations[r*width+c], 8192 + 1700*4)
            column = int((RAMP_X+256-ORIGIN)/128)
            expected = 1700 - 256*math.tan(math.radians(angle))
            self.assertAlmostEqual((elevations[row*width+column]-8192)/4, expected, delta=.13)

    def test_invalid_course_is_rejected(self):
        for angles, rise in (((55, 60),1600), ((55, 60, float('nan')),1600),
                             ((40,60,65),1600), ((55,60,65),5000), ((55,60,65),float('inf'))):
            with self.assertRaises(ValueError):
                build(angles, rise)

    def test_map_is_original_and_reproducible(self):
        # No borrowed map is a build input; this exact file set is the distributable artifact.
        raw, files = build((55,60,65),1600)
        self.assertEqual(raw, build((55,60,65),1600)[0])
        self.assertEqual(set(files), {'war3map.w3i','war3map.w3e','war3map.wpm','war3map.j',
            'war3map.doo','war3mapUnits.doo','war3map.w3r','war3map.w3c','war3map.w3s',
            'war3map.mmp','war3map.shd','war3mapMap.tga','war3mapPreview.tga'})
        self.assertNotIn(b'@LANE_SETUP@', files['war3map.j'])
        self.assertIn(b'F6: FPS', files['war3map.j'])


if __name__ == '__main__':
    unittest.main()
