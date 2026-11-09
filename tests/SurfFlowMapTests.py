"""Prove physical course continuity, closed authored solids and valid shared map logic."""
from collections import Counter
from pathlib import Path
import math
import re
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from build_surf_flow import build
from surf_flow_layout import HALF_PAD, HALF_RAIL, PADS, PIT, spans
from surf_flow_models import solid


class SurfFlowMapTests(unittest.TestCase):
    def test_every_stage_joins_both_decks_and_all_seams(self):
        pieces=spans()
        self.assertEqual(len(pieces),40)
        for stage,(start,end) in enumerate(zip(PADS,PADS[1:]),1):
            rows=[s for s in pieces if s.stage==stage]
            self.assertEqual(rows[0].bank0,0)
            self.assertEqual(rows[-1].bank1,0)
            north=start.direction==90
            axis=lambda s:s.y if north else s.x
            self.assertAlmostEqual(axis(rows[0])-rows[0].length/2,(start.y if north else start.x)+HALF_PAD)
            self.assertAlmostEqual(axis(rows[-1])+rows[-1].length/2,(end.y if north else end.x)-HALF_PAD)
            self.assertAlmostEqual(rows[0].z-rows[0].drop/2,start.z)
            self.assertAlmostEqual(rows[-1].z+rows[-1].drop/2,end.z)
            for span in rows:
                for across in (-HALF_RAIL,-120,0,120,HALF_RAIL):
                    ratio=abs(across)/HALF_RAIL
                    entry=span.z-span.drop/2-span.bank0*ratio
                    exit=span.z+span.drop/2-span.bank1*ratio
                    self.assertLessEqual(exit,entry+.001,'Flattening must not create an uphill speed trap')
            for left,right in zip(rows,rows[1:]):
                self.assertAlmostEqual(axis(left)+left.length/2,axis(right)-right.length/2)
                # The whole bank profile must meet, not merely its ridge or model box.
                for across in (-HALF_RAIL,-120,0,120,HALF_RAIL):
                    ratio=abs(across)/HALF_RAIL
                    self.assertAlmostEqual(left.z+left.drop/2-left.bank1*ratio,
                                           right.z-right.drop/2-right.bank0*ratio)
        self.assertGreater(HALF_PAD,HALF_RAIL+64)
        self.assertGreater(min(p.z for p in PADS),-1850)
        self.assertGreater(-1850,PIT)
        self.assertTrue(0<=8192+PIT*4<16384)

    def test_real_solids_remain_closed_and_discoverable(self):
        for span in spans():
            edges=Counter()
            for triangle in solid(span):
                a,b,c=triangle
                u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
                area=sum(n*n for n in (u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]))
                self.assertGreater(area,1)
                for first,second in ((a,b),(b,c),(c,a)):edges[tuple(sorted((first,second)))]+=1
            self.assertTrue(all(count==2 for count in edges.values()))
            # Native discovery searches from the object origin, not every visual vertex.
            self.assertLess(math.hypot(span.length/2,HALF_RAIL)+64,600)

    def test_generated_map_and_shared_timer_contract(self):
        raw,files=build()
        self.assertEqual(raw,build()[0])
        self.assertEqual(raw[:4],b'HM3W')
        script=files['war3map.j'].decode('utf-8')
        self.assertNotRegex(script,r'@[A-Z_]+@')
        self.assertIn('Warcraft CS - Surf Flow',script)
        self.assertIn('courseStartEdge=768.',script)
        self.assertIn('courseFall=-1850.',script)
        self.assertNotIn('AIR GAPS',script)
        self.assertIn('not practice and (best == 0. or result < best)',script)
        # Returns are explicit failure/manual actions, never required stage-to-stage links.
        tick=script.split('function SkylineTick')[1].split('function SkylineSolid')[0]
        self.assertEqual(tick.count('call SkylineReturn()'),1)
        self.assertIn('if automatic and z < courseFall',tick)
        self.assertEqual(sum(name.endswith('.mdx') for name in files),42)
        code=re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*','',script)
        self.assertTrue(all(len(x)<=9 for x in re.findall(r'\d+\.(\d+)',code)))


if __name__=='__main__':unittest.main()
