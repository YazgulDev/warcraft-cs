"""Check the distributable course, real solid faces and classic interpreter limits."""
from collections import Counter
import math
from pathlib import Path
import re
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from build_surf_course import build, CELLS, ORIGIN, PIT
from surf_course_layout import PADS, rails


def chunks(data):
    at = 4
    result = {}
    while at < len(data):
        tag, length = struct.unpack_from('<4sI', data, at)
        result[tag] = data[at+8:at+8+length]
        at += 8+length
    if at != len(data):
        raise ValueError('Truncated MDX chunk')
    return result


def vertices(data):
    geo = chunks(data)[b'GEOS']
    count = struct.unpack_from('<I', geo, 8)[0]
    return [struct.unpack_from('<3f', geo, 12+i*12) for i in range(count)]


class SurfCourseMapTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.files = build()

    def test_portable_original_container_and_imports(self):
        self.assertEqual(self.raw[:4], b'HM3W')
        self.assertEqual(self.raw[512:516], b'MPQ\x1a')
        self.assertEqual(self.raw, build()[0])
        imported = {name for name in self.files if name.startswith('WarcraftCS/')}
        self.assertEqual(len(imported), 8)
        for name in imported:
            self.assertIn(name.replace('/', '\\').encode()+b'\0', self.files['war3map.imp'])
        # No native module, game archive or borrowed map can enter this downloadable course.
        self.assertTrue(all(name.startswith('war3map') or name in imported for name in self.files))
        self.assertLess(len(self.raw), 2_000_000)
        script = self.files['war3map.j'].decode('utf-8')
        self.assertNotIn('@COURSE_DATA@', script)
        self.assertIn('F6: FPS', script)

    def test_terrain_height_does_not_wrap_or_hide_a_floor(self):
        data = self.files['war3map.w3e']
        tile_count = struct.unpack_from('<I', data, 13)[0]
        at = 17+tile_count*4
        cliff_count = struct.unpack_from('<I', data, at)[0]
        at += 4+cliff_count*4
        width, height, ox, oy = struct.unpack_from('<IIff', data, at)
        self.assertEqual((width, height, ox, oy), (CELLS+1, CELLS+1, ORIGIN, ORIGIN))
        at += 16
        self.assertEqual(len(data)-at, width*height*7)
        values = {struct.unpack_from('<H', data, at+i*7)[0] for i in range(width*height)}
        self.assertEqual(values, {8192+PIT*4})
        self.assertTrue(0 <= min(values) < 16384)
        self.assertLess(PIT, min(p.z for p in PADS)-500)

    def test_closed_solids_and_real_surf_normals(self):
        for name, data in self.files.items():
            if not name.endswith('.mdx'):
                continue
            self.assertEqual(data[:4], b'MDLX')
            records = chunks(data)
            self.assertEqual(struct.unpack('<I', records[b'VERS'])[0], 800)
            points = vertices(data)
            self.assertTrue(all(math.isfinite(v) for p in points for v in p))
            edges = Counter()
            steep = 0
            for offset in range(0, len(points), 3):
                a, b, c = points[offset:offset+3]
                u, v = [b[i]-a[i] for i in range(3)], [c[i]-a[i] for i in range(3)]
                normal = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
                length = math.sqrt(sum(n*n for n in normal))
                self.assertGreater(length, 1)
                steep += math.cos(math.radians(80)) < normal[2]/length < math.cos(math.radians(46))
                for first, second in ((a,b),(b,c),(c,a)):
                    edges[tuple(sorted((first,second)))] += 1
            self.assertTrue(all(count == 2 for count in edges.values()), name)
            self.assertEqual(steep > 0, '/Rail' in name)
        finish = vertices(self.files['WarcraftCS/Surf/FinishPad.mdx'])
        self.assertGreaterEqual(max(p[0] for p in finish)-min(p[0] for p in finish), 1400)

    def test_route_fits_map_and_has_air_gaps(self):
        route = rails()
        self.assertEqual({r.stage for r in route}, {1,2,3})
        self.assertEqual(len(PADS), 4)
        for r in route:
            self.assertLess(abs(r.x)+448, 7808)
            self.assertLess(abs(r.y)+448, 7808)
            self.assertGreater(r.z, PIT)
        gaps = [b.x-a.x-512 for a,b in zip(route[12:],route[13:])]
        self.assertEqual(sum(gap > 0 for gap in gaps), 2)
        self.assertTrue(all(0 <= gap <= 192 for gap in gaps))

    def test_jass_fractional_literals_fit_classic_integer_parser(self):
        # Classic JASS uses a 32-bit fractional accumulator/divisor. An 11-digit
        # degrees conversion silently moved north-facing respawns to the wrong rail face.
        script = self.files['war3map.j'].decode('utf-8')
        code = re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*', '', script)
        for fraction in re.findall(r'\d+\.(\d+)', code):
            self.assertLessEqual(len(fraction), 9)
            self.assertLessEqual(int(fraction), 2**31-1)
        angle = float(re.search(r'Sin\(cpFacing\[checkpoint\]\*([\d.]+)\)', code)[1])
        self.assertAlmostEqual(100*math.sin(90*angle), 100, places=4)
        self.assertAlmostEqual(100*math.cos(90*angle), 0, places=4)


if __name__ == '__main__':
    unittest.main()
