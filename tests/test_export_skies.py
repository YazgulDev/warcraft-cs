"""Exercise private sky decoding with synthetic, project-owned pixels only."""
import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("export_skies", Path(__file__).resolve().parents[1] / "tools/export_skies.py")
skies = importlib.util.module_from_spec(spec)
spec.loader.exec_module(skies)


class SkiesTest(unittest.TestCase):
    def test_orientation_rle_and_package(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            env = root / "gfx/env"
            env.mkdir(parents=True)
            for face in skies.FACES:
                header = bytearray(18)
                header[2] = 10
                struct.pack_into("<HHBB", header, 12, 2, 2, 24, 0)
                # Bottom-left storage, two solid RLE rows: blue below red.
                (env / ("test" + face + ".tga")).write_bytes(header + bytes([129, 255, 0, 0, 129, 0, 0, 255]))
            w, h, rgba = skies.read_tga(env / "testup.tga")
            self.assertEqual((w, h), (2, 2))
            self.assertEqual(rgba[:4], bytes([255, 0, 0, 255]))
            manifest = skies.export(root, root / "output")
            self.assertEqual(len(manifest), 1)
            data = (root / "output/test.wcs").read_bytes()
            self.assertEqual(data[:12], struct.pack("<4sII", b"WCS1", 2, 2))
            self.assertEqual(len(data), 12 + 6 * 2 * 2 * 4)
            (env / "testup.tga").write_bytes(b"truncated")
            with self.assertRaises(ValueError):
                skies.read_tga(env / "testup.tga")


if __name__ == "__main__":
    unittest.main()
