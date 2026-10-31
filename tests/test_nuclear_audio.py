"""Synthetic tests for nuclear mix timing, PCM, headroom, owner-source policy and update behavior."""
from pathlib import Path
import hashlib
import io
import json
import math
import struct
import sys
import tempfile
import unittest
import wave
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import export_nuclear_audio as mix


def pcm(seconds=.2, rate=8000, width=2, channels=1, tone=500):
    output = io.BytesIO(); data = bytearray()
    for i in range(int(seconds * rate)):
        value = .9 * math.sin(2 * math.pi * tone * i / rate)
        for _ in range(channels):
            data += bytes([round(value * 127 + 128)]) if width == 1 else round(value * ((1 << (width * 8 - 1)) - 1)).to_bytes(width, 'little', signed=True)
    with wave.open(output, 'wb') as audio:
        audio.setparams((channels, width, rate, 0, 'NONE', 'not compressed')); audio.writeframes(data)
    return output.getvalue()


def pak(path, files):
    payload = bytearray(b'PACK' + bytes(8)); directory = bytearray()
    for name, data in files.items():
        at = len(payload); payload.extend(data)
        directory.extend(name.encode().ljust(56, b'\0') + struct.pack('<II', at, len(data)))
    struct.pack_into('<II', payload, 4, len(payload), len(directory)); payload.extend(directory); path.write_bytes(payload)


class NuclearMixTests(unittest.TestCase):
    def test_pcm_formats_and_pitch(self):
        for width in (1, 2, 3, 4):
            for channels in (1, 2):
                data, rate = mix.decode(pcm(width=width, channels=channels))
                self.assertEqual(len(data), 1600); self.assertEqual(rate, 8000)
                slowed = mix.resample(data, rate, .75)
                self.assertAlmostEqual(len(slowed) / mix.RATE, .2 / .75, delta=.001)
                crossings = sum(a <= 0 < b for a, b in zip(slowed, slowed[1:]))
                self.assertAlmostEqual(crossings / (len(slowed) / mix.RATE), 375, delta=8)
        with self.assertRaises((ValueError, wave.Error)):
            mix.decode(pcm()[:-20])

    def test_headroom_tail_and_mute(self):
        inputs = {key: pcm() for key in mix.SOURCES}; settings = mix.settings(None)
        data, report = mix.compose(inputs, settings)
        with wave.open(io.BytesIO(data), 'rb') as audio:
            self.assertEqual((audio.getnchannels(), audio.getsampwidth(), audio.getframerate()), (1, 2, 44100))
            samples = struct.unpack('<' + str(audio.getnframes()) + 'h', audio.readframes(audio.getnframes()))
        self.assertAlmostEqual(report['seconds'], 8.45, places=3)
        self.assertLessEqual(max(abs(x) for x in samples), 30147); self.assertEqual(samples[-1], 0)
        self.assertLess(sum(x*x for x in samples[-4410:]), sum(x*x for x in samples[-13230:-8820]))
        for key in list(settings):
            if key.endswith('Volume'): settings[key] = 0
        muted, report = mix.compose(inputs, settings)
        with wave.open(io.BytesIO(muted), 'rb') as audio:
            self.assertFalse(any(audio.readframes(audio.getnframes())))
        self.assertEqual(report['peak_after'], 0)

    def test_config_bounds_and_export(self):
        # Every temporary directory is created under the explicit project-owned test root.
        root = ROOT / 'build'; root.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='nuclear-audio-', dir=root) as name:
            work = Path(name).resolve(); self.assertTrue(work.is_relative_to(root.resolve()))
            cs, valve, out = work/'owned/cstrike', work/'owned/valve', work/'private/assets'
            cs.mkdir(parents=True); valve.mkdir(parents=True)
            config = work/'WarcraftCS.ini'
            config.write_text('[nUcLeArAuDiO]\nC4Volume=999\nMortarPitch=nan\nRumbleSeconds=-1\nDebrisVolume=0\n')
            tuning = mix.settings(config)
            self.assertEqual(tuning['C4Volume'], 1); self.assertEqual(tuning['MortarPitch'], .72)
            self.assertEqual(tuning['RumbleSeconds'], 0); self.assertEqual(tuning['DebrisVolume'], 0)
            sources = {key: pcm() for key in mix.SOURCES}
            for game, folder in [('cstrike', cs), ('valve', valve)]:
                pak(folder/'pak0.pak', {path: sources[key] for key,(owner,path) in mix.SOURCES.items() if owner==game})
            # Owner loose-file overrides take precedence over the shipped PAK entry.
            loose=cs/'sound/weapons/c4_explode1.wav'; loose.parent.mkdir(parents=True); loose.write_bytes(pcm(tone=700))
            result = mix.export(cs, out, config); self.assertTrue(result['available'])
            self.assertEqual(result['sources']['C4']['sha256'], hashlib.sha256(loose.read_bytes()).hexdigest())
            target=out/'sounds/nuclear_blast.wav'; first=target.read_bytes()
            self.assertEqual(mix.export(cs, out, config)['sha256'], hashlib.sha256(first).hexdigest())
            unrelated=out/'keep.txt'; unrelated.write_text('preserve custom cache')
            (valve/'pak0.pak').unlink(); self.assertFalse(mix.export(cs, out, config)['available'])
            self.assertFalse(target.exists()); self.assertFalse((out/'sounds/nuclear_blast.wav.tmp').exists())
            self.assertEqual(unrelated.read_text(), 'preserve custom cache')
            with self.assertRaises(ValueError): mix.export(cs, cs/'sound/private')


if __name__ == '__main__': unittest.main()
