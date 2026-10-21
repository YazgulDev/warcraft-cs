"""Synthetic studio-model normals/materials/local skeleton conversion and hand-free WCG3 previews."""
import struct
import tempfile
import unittest
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from export_models import StudioModel
from export_buy_previews import export_preview

def fixture(path):
    data=bytearray(244)
    def add(size):
        at=len(data);data.extend(bytes(size));return at
    def integer(at,value):struct.pack_into('<i',data,at,value)
    data[:4]=b'IDST';integer(4,10)
    bones=add(224);integer(140,2);integer(144,bones)
    integer(bones+32,-1);integer(bones+112+32,0);struct.pack_into('<f',data,bones+112+64,10)
    textures=add(160);integer(180,2);integer(184,textures)
    for i,name in enumerate((b'chrome.bmp',b'GLOVED_sleeve.bmp')):
        at=textures+i*80;data[at:at+len(name)]=name;pixels=add(1+768);struct.pack_into('<4i',data,at+64,3 if i==0 else 0,1,1,pixels)
    skins=add(4);integer(192,2);integer(196,1);integer(200,skins);struct.pack_into('<2h',data,skins,0,1)
    bodies=add(76);integer(204,1);integer(208,bodies);models=add(112);struct.pack_into('<3i',data,bodies+64,1,1,models)
    meshes=add(40);integer(models+72,2);integer(models+76,meshes)
    vertex_bones=add(3);data[vertex_bones:vertex_bones+3]=b'\x01'*3
    vertices=add(36);struct.pack_into('<9f',data,vertices,0,0,0,0,1,0,0,0,1)
    normal_bones=add(1);data[normal_bones]=1;normals=add(12);struct.pack_into('<3f',data,normals,0,0,1)
    for at,value in ((80,3),(84,vertex_bones),(88,vertices),(92,1),(96,normal_bones),(100,normals)):integer(models+at,value)
    for i in range(2):
        commands=add(28);struct.pack_into('<h',data,commands,3)
        for j in range(3):struct.pack_into('<4h',data,commands+2+j*8,j,0,0,0)
        struct.pack_into('<5i',data,meshes+i*20,1,commands,i,1,0)
    sequence=add(176);integer(164,1);integer(168,sequence);data[sequence:sequence+4]=b'idle';struct.pack_into('<f',data,sequence+32,30)
    integer(sequence+36,1);integer(sequence+56,1);integer(sequence+120,1);integer(sequence+124,add(24))
    integer(72,len(data));path.write_bytes(data)

class StudioPresentationTests(unittest.TestCase):
    def test_modern_metadata_and_preview(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/'build') as folder:
            root=Path(folder);src=root/'owned.mdl';fixture(src);model=StudioModel(src);dst=root/'held.wcg'
            model.export(dst,presentation=True);data=dst.read_bytes()
            self.assertEqual(data[:4],b'WCG3');self.assertEqual(struct.unpack_from('<2i',data,20),(-1,0))
            self.assertEqual(struct.unpack_from('<I',data,28)[0],3)
            out=root/'buy.wcg';self.assertEqual(export_preview(dst,out,model.texture_names(),('GLOVED_sleeve.bmp',)),1)
            self.assertEqual(struct.unpack_from('<4s4I',out.read_bytes()),(b'WCG3',2,1,1,2))
            # The legacy default is retained for CS/RPG; normals/materials are strictly opt-in.
            model.export(root/'legacy.wcg');self.assertEqual((root/'legacy.wcg').read_bytes()[:4],b'WCG2')
            self.assertNotEqual(data,(root/'legacy.wcg').read_bytes())
if __name__=='__main__':unittest.main()
