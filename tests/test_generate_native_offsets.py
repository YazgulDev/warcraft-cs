"""Reject incompatible native declarations before replacing a usable generated table."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile
import unittest

GENERATOR = Path(__file__).resolve().parents[1] / "tools/generate_native_offsets.py"
BUILD = GENERATOR.parent.parent / "build/host-generator-tests"


class NativeTableGenerationTests(unittest.TestCase):
    def generate(self, declaration, natives, signatures):
        BUILD.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="case-", dir=BUILD) as directory:
            # TemporaryDirectory only removes this freshly created directory inside the explicit build root.
            root = Path(directory).resolve()
            self.assertTrue(root.is_relative_to(BUILD.resolve()))
            reference, source, output = root / "reference.inc", root / "natives.json", root / "output.inc"
            reference.write_text(declaration, encoding="utf-8")
            source.write_text(json.dumps({"natives": natives, "signatures": signatures}), encoding="utf-8")
            output.write_text("existing verified table", encoding="utf-8")
            result = subprocess.run([sys.executable, str(GENERATOR), "--reference", str(reference),
                                     "--natives", str(source), "--output", str(output),
                                     "--version", "1.27.0.52240"], capture_output=True, text=True)
            return result.returncode, output.read_text(encoding="utf-8")

    def test_name_and_signature_must_both_match(self):
        code, text = self.generate("NATIVE(Bits, GetUnitX, (Handle), 0x3C5510)",
                                   {"GetUnitX": 0x1E66B0}, {"GetUnitX": "(Hunit;)R"})
        self.assertEqual(code, 0)
        self.assertIn("0x1E66B0", text)

    def test_wrong_return_type_preserves_previous_output(self):
        code, text = self.generate("NATIVE(Bits, GetUnitX, (Handle), 0x3C5510)",
                                   {"GetUnitX": 0x1E66B0}, {"GetUnitX": "(Hunit;)I"})
        self.assertNotEqual(code, 0)
        self.assertEqual(text, "existing verified table")

    def test_missing_entry_preserves_previous_output(self):
        code, text = self.generate("NATIVE(Bits, GetUnitX, (Handle), 0x3C5510)", {}, {})
        self.assertNotEqual(code, 0)
        self.assertEqual(text, "existing verified table")

    def test_unsigned_handle_alias_is_checked(self):
        declaration = "NATIVE(void, SelectUnit, (uint32_t,BOOL), 0x3C7910)"
        code, _ = self.generate(declaration, {"SelectUnit": 0x1F3D10}, {"SelectUnit": "(Hunit;B)V"})
        self.assertEqual(code, 0)
        code, text = self.generate(declaration, {"SelectUnit": 0x1F3D10}, {"SelectUnit": "(R B)V"})
        self.assertNotEqual(code, 0)
        self.assertEqual(text, "existing verified table")


if __name__ == "__main__":
    unittest.main()
