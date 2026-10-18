"""Synthetic GoldSrc attachment fixtures, without any game data."""
import struct
import tempfile
import unittest
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from export_models import StudioModel, export_world_grenades

def fixture(path, count=3, bone=0):
    data=bytearray(244+112+80+769+count*88)
    data[:4]=b'IDST';struct.pack_into('<i',data,4,10);struct.pack_into('<i',data,72,len(data))
    for at,value in {140:1,144:244,164:0,168:len(data),180:1,184:356,192:0,200:436,212:count,216:1205}.items():
        struct.pack_into('<i',data,at,value)
    struct.pack_into('<i',data,244+32,-1)
    data[356:364]=b'body.bmp';struct.pack_into('<4i',data,356+64,0,1,1,436)
    for i in range(count):struct.pack_into('<ii3f',data,1205+i*88+32,0,bone,20+i,4,2)
    path.write_bytes(data)

class MuzzleExportTests(unittest.TestCase):
    def test_world_models_use_owned_world_sources(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);(root/'models').mkdir();output=root/'output'
            for name in ('hegrenade','flashbang','smokegrenade'):fixture(root/'models'/f'w_{name}.mdl',0)
            entries=export_world_grenades(root,output)
            self.assertEqual([item['model'] for item in entries],['w_hegrenade.mdl','w_flashbang.mdl','w_smokegrenade.mdl'])
            for name in ('hegrenade','flashbang','smokegrenade'):
                self.assertTrue((output/'world'/f'{name}.wcg').read_bytes().endswith(b'WCM1'+bytes(4)))
    def test_selected_and_dual_attachments(self):
        with tempfile.TemporaryDirectory() as folder:
            src=Path(folder)/'source.mdl';dst=Path(folder)/'gun.wcg';fixture(src)
            model=StudioModel(src)
            for chosen in [(0,),(2,),(0,2)]:
                model.export(dst,muzzle_attachments=chosen);data=dst.read_bytes();at=data.rindex(b'WCM1')
                self.assertEqual(struct.unpack_from('<I',data,at+4)[0],len(chosen))
                for n,index in enumerate(chosen):self.assertEqual(struct.unpack_from('<I3f',data,at+8+n*16),(0,20+index,4,2))
    def test_missing_and_invalid_attachments(self):
        with tempfile.TemporaryDirectory() as folder:
            src=Path(folder)/'source.mdl';dst=Path(folder)/'gun.wcg';fixture(src,0)
            StudioModel(src).export(dst);self.assertTrue(dst.read_bytes().endswith(b'WCM1'+bytes(4)))
            fixture(src,bone=99)
            with self.assertRaises(ValueError):StudioModel(src).export(dst)
            fixture(src)
            with self.assertRaises(ValueError):StudioModel(src).export(dst,muzzle_attachments=(9,))

if __name__=='__main__':unittest.main()
