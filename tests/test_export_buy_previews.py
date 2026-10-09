"""Exercise hand filtering and rig preservation using only synthetic mesh bytes."""
import struct
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from export_buy_previews import export_preview

class PreviewTests(unittest.TestCase):
    def test_weapon_survives_without_hands_or_silencer(self):
        with tempfile.TemporaryDirectory() as folder:
            source=Path(folder)/'source.wcg';output=Path(folder)/'buy/weapon.wcg'
            texture=struct.pack('<2I4B',1,1,200,100,50,255)
            triangle=struct.pack('<5fI',1,2,3,.5,.5,0)*3
            meshes=b''.join(struct.pack('<2I',index,3)+triangle for index in range(3))
            animation=struct.pack('<32sfI12f',b'idle',1,1,*[1,0,0,0,0,1,0,0,0,0,1,0])
            source.write_bytes(struct.pack('<4s4I',b'WCG2',3,3,1,1)+texture*3+meshes+animation)
            self.assertEqual(export_preview(source,output,['view_glove.bmp','receiver.bmp','silencer.bmp']),1)
            data=output.read_bytes()
            self.assertEqual(struct.unpack_from('<4s4I',data),(b'WCG2',3,1,1,1))
            self.assertEqual(data[20:56],texture*3)
            self.assertEqual(data[56:64],struct.pack('<2I',1,3))
            self.assertTrue(data.endswith(animation))
            # All-hand sources cannot pretend to contain a weapon preview.
            with self.assertRaises(ValueError):
                export_preview(source,output,['view_glove.bmp','view_skin.BMP','silencer.bmp'])
    def test_truncated_geometry_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            source=Path(folder)/'bad.wcg'
            source.write_bytes(struct.pack('<4s4I',b'WCG2',1,1,1,1)+struct.pack('<2I4B',1,1,0,0,0,255)+struct.pack('<2I',0,99999))
            with self.assertRaises(ValueError): export_preview(source,Path(folder)/'out.wcg',['weapon.bmp'])

if __name__=='__main__': unittest.main()
