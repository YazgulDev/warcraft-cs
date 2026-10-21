"""Synthetic optional models/PAK/sounds/seals; never include retail fixtures in the repository."""
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from test_optional_rpg import Model as RpgModel, OptionalRpgTests
import export_halflife_weapons as hl

class Model(RpgModel):
    def audio_timeline(self,path):
        super().audio_timeline(path)
        return set() # The production exporter returns the set of studio-event sample names.


class HalfLifeConversionTests(unittest.TestCase):
    def test_independent_loose_and_pak_weapons(self):
        for packed in (False, True):
            with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1] / 'build') as folder, patch('export_models.StudioModel', Model):
                root=Path(folder);(root/'valve').mkdir();cs=root/'cstrike';cs.mkdir();out=root/'out'
                helper=OptionalRpgTests();sound=next(v for k,v in helper.resources().items() if k.endswith('.wav'))
                files={f'models/v_{stem}.mdl': b'owned-model' for stem in hl.WEAPONS}
                files.update({hl.sound_source(name): sound for names in hl.WEAPONS.values() for name in names})
                self.assertTrue(all(not result['available'] for result in hl.export(cs,out)))
                if packed:helper.write_pak(root/'valve/pak0.pak',files)
                else:
                    for name,data in files.items():
                        path=root/'valve'/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
                self.assertTrue(all(result['available'] for result in hl.export(cs,out)))
                for stem in hl.WEAPONS:
                    seal=(out/f'{stem}.ready').read_bytes();self.assertEqual(seal[:4],b'WHL1')
                    for i,name in enumerate(hl.outputs(stem)):
                        data=(out/name).read_bytes()
                        self.assertEqual(struct.unpack_from('<QQ',seal,4+i*16),(len(data),hl.fingerprint(data)))
                    self.assertEqual(struct.unpack_from('<I',(out/f'buy/{stem}.wcg').read_bytes(),8)[0],1)
                # One corrupt loose override invalidates only Tau, including when a valid PAK remains.
                corrupt=root/'valve/models/v_gauss.mdl';corrupt.parent.mkdir(parents=True,exist_ok=True);corrupt.write_bytes(b'corrupt')
                results={result['weapon']:result['available'] for result in hl.export(cs,out)}
                self.assertEqual(results,{'crowbar':True,'gauss':False,'egon':True});self.assertFalse((out/'gauss.ready').exists())


if __name__=='__main__':unittest.main()
