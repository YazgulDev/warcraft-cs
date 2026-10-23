"""Synthetic optional models/PAK/sounds/seals; never include retail fixtures in the repository."""
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from test_optional_rpg import Model as RpgModel, OptionalRpgTests
import export_halflife_weapons as hl
import export_energy_sprites as sprites

class Model(RpgModel):
    def audio_timeline(self,path):
        super().audio_timeline(path)
        return set() # The production exporter returns the set of studio-event sample names.


class HalfLifeConversionTests(unittest.TestCase):
    def test_additive_sprite_pixels_frames_and_invalid_inputs(self):
        palette=bytes(range(256))*3
        header=struct.pack('<4siiifiiifi',b'IDSP',2,2,1,32.,2,1,2,0.,1)+struct.pack('<H',256)+palette
        frame=struct.pack('<iiiii',0,0,0,2,1)+bytes([3,17])
        original=header+frame+frame
        cache=sprites.decode(original)
        self.assertEqual(cache[:8],b'WES1'+struct.pack('<I',2))
        self.assertEqual(cache[16:24],palette[9:12]+b'\xff'+palette[51:54]+b'\xff')
        for malformed in (b'',original[:-1],original+b'x',original[:810]+struct.pack('<i',1)+original[814:]):
            with self.assertRaises((ValueError,struct.error)):sprites.decode(malformed)
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);(root/'valve/sprites').mkdir(parents=True);out=root/'out'
            owned=root/'valve/sprites/xbeam1.spr';owned.write_bytes(original)
            sprites.export(root/'cstrike',out);self.assertEqual((out/'effects/egon_beam.wes').read_bytes(),cache)
            owned.write_bytes(b'broken');sprites.export(root/'cstrike',out)
            self.assertFalse((out/'effects/egon_beam.wes').exists())

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
