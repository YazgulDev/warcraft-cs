"""Optional install/PAK/seal behavior with synthetic resources, never game assets."""
import io
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import export_halflife_rpg as rpg


class Model:
    def __init__(self, path):
        if path.read_bytes()==b'corrupt':raise ValueError('Synthetic malformed model')
    def integer(self, at):return 0
    def texture_names(self):return ['body.bmp','GLOVED_sleeve.bmp']
    def export(self, path, **_):
        texture=struct.pack('<2I4B',1,1,255,255,255,255)
        triangle=struct.pack('<5fI',1,2,3,.5,.5,0)*3
        meshes=b''.join(struct.pack('<2I',i,3)+triangle for i in range(2))
        animation=struct.pack('<32sfI12f',b'idle',1,1,*[1,0,0,0,0,1,0,0,0,0,1,0])
        path.write_bytes(struct.pack('<4s4I',b'WCG2',2,2,1,1)+texture*2+meshes+animation)
    def audio_timeline(self, path):path.write_bytes(b'WCA1'+bytes(4))


class OptionalRpgTests(unittest.TestCase):
    def resources(self):
        data=io.BytesIO()
        with wave.open(data,'wb') as sound:
            sound.setnchannels(1);sound.setsampwidth(1);sound.setframerate(8000);sound.writeframes(b'\x80'*16)
        return {name:b'owned-model' if name.endswith('.mdl') else data.getvalue() for name in rpg.SOURCES}
    def write_pak(self, path, files):
        data=bytearray();directory=bytearray()
        for name, content in files.items():
            directory+=struct.pack('<56sII',name.encode(),12+len(data),len(content));data+=content
        path.write_bytes(b'PACK'+struct.pack('<II',12+len(data),len(directory))+data+directory)
    def test_absent_partial_complete_and_removed_resources(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/"build") as folder,patch('export_models.StudioModel',Model):
            root=Path(folder);cs=root/'cstrike';cs.mkdir();out=root/'assets';valve=root/'valve';valve.mkdir()
            self.assertFalse(rpg.export(cs,out)['available'])
            files=self.resources()
            for name,data in files.items():
                path=valve/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
            result=rpg.export(cs,out);self.assertTrue(result['available'],result)
            seal=(out/'rpg.ready').read_bytes();self.assertEqual(len(seal),4+16*len(rpg.OUTPUTS))
            for i,name in enumerate(rpg.OUTPUTS):
                data=(out/name).read_bytes();self.assertEqual(struct.unpack_from('<QQ',seal,4+i*16),(len(data),rpg.fingerprint(data)))
            self.assertEqual(struct.unpack_from('<I',(out/'buy/rpg.wcg').read_bytes(),8)[0],1) # Glove mesh removed.
            (valve/rpg.SOURCES[-1]).unlink();self.assertFalse(rpg.export(cs,out)['available']);self.assertFalse((out/'rpg.ready').exists())
            (valve/rpg.SOURCES[-1]).write_bytes(files[rpg.SOURCES[-1]]);(valve/rpg.SOURCES[0]).write_bytes(b'corrupt')
            self.assertFalse(rpg.export(cs,out)['available']);self.assertFalse((out/'rpg.ready').exists())
    def test_pak_detection_and_loose_override(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/"build") as folder,patch('export_models.StudioModel',Model):
            root=Path(folder);(root/'valve').mkdir();(root/'cstrike').mkdir();files=self.resources();self.write_pak(root/'valve/pak0.pak',files)
            result=rpg.export(root/'cstrike',root/'out');self.assertTrue(result['available'],result)
            path=root/'valve'/rpg.SOURCES[0];path.parent.mkdir(parents=True);path.write_bytes(b'loose')
            self.assertEqual(rpg.OwnedFiles(root/'valve').read(rpg.SOURCES[0]),b'loose')
            (root/'valve/pak1.pak').write_bytes(b'PACK'+struct.pack('<II',999999,64))
            with self.assertRaises(ValueError):rpg.OwnedFiles(root/'valve').read('missing')


if __name__=='__main__':unittest.main()
