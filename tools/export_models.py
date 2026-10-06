"""Convert the owner's GoldSrc viewmodels into a private Warcraft-CS mesh cache."""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path

import numpy as np
from greatsword import export_greatsword


class StudioModel:
    """Read version-10 studio models without importing engine or retail code."""

    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if self.data[:4] != b"IDST" or self.integer(4) != 10:
            raise ValueError(f"Not a GoldSrc v10 studio model: {path}")
        if self.integer(72) != len(self.data):
            raise ValueError(f"Truncated studio model: {path}")
        self.bones = []
        for i in range(self.integer(140)):
            at = self.integer(144) + i * 112
            self.bones.append((self.integer(at + 32), self.floats(at + 64, 6), self.floats(at + 88, 6)))

    def integer(self, at: int) -> int:
        return struct.unpack_from("<i", self.data, at)[0]

    def floats(self, at: int, count: int) -> np.ndarray:
        return np.array(struct.unpack_from(f"<{count}f", self.data, at))

    @staticmethod
    def channel(data: bytes, at: int, frame: int) -> int:
        # Animation channels repeat their last valid sample within each RLE span.
        while True:
            valid, total = data[at:at + 2]
            if total == 0 or valid > total:
                raise ValueError("Invalid studio animation run")
            if frame < total:
                return struct.unpack_from("<h", data, at + 2 * (min(frame, valid - 1) + 1))[0]
            frame -= total
            at += (valid + 1) * 2

    @staticmethod
    def matrix(values: np.ndarray) -> np.ndarray:
        # GoldSrc stores XYZ Euler rotations; hierarchy composition yields world bones.
        x, y, z = values[3:]
        sx, sy, sz = np.sin([x, y, z])
        cx, cy, cz = np.cos([x, y, z])
        result = np.eye(4)
        result[:3, :3] = [[cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz],
                          [cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz],
                          [-sy, sx * cy, cx * cy]]
        result[:3, 3] = values[:3]
        return result

    def sequences(self) -> list:
        sequences = []
        for i in range(self.integer(164)):
            at = self.integer(168) + i * 176
            name = self.data[at:at + 32].split(b"\0")[0]
            fps = self.floats(at + 32, 1)[0]
            frames = self.integer(at + 56)
            anim = self.integer(at + 124)
            group = self.integer(at + 156)
            source = self.data
            if group:
                # External animation groups are read only from the same owned installation.
                group_at = self.integer(176) + group * 104
                group_name = self.data[group_at + 32:group_at + 96].split(b"\0")[0].decode("ascii")
                source = (self.path.parent / Path(group_name.replace("\\", "/")).name).read_bytes()
            matrices = []
            for frame in range(frames):
                world = []
                for bone, (parent, base, scale) in enumerate(self.bones):
                    cursor = anim + bone * 12
                    offsets = struct.unpack_from("<6H", source, cursor)
                    values = base.copy()
                    for channel, offset in enumerate(offsets):
                        if offset:
                            values[channel] += self.channel(source, cursor + offset, frame) * scale[channel]
                    transform = self.matrix(values)
                    if parent >= 0:
                        transform = world[parent] @ transform
                    world.append(transform)
                matrices.extend(matrix[:3, :].reshape(-1) for matrix in world)
            sequences.append((name, float(fps), frames, np.asarray(matrices, dtype="<f4").tobytes()))
        return sequences

    def export(self, destination: Path, excluded_textures: tuple[str, ...] = ()) -> dict:
        textures = []
        texture_names = []
        excluded = {name.casefold() for name in excluded_textures}
        excluded_meshes = 0
        for i in range(self.integer(180)):
            at = self.integer(184) + i * 80
            texture_names.append(self.data[at:at + 64].split(b"\0")[0].decode("ascii"))
            flags, width, height, pixels = struct.unpack_from("<4i", self.data, at + 64)
            if not (0 < width <= 2048 and 0 < height <= 2048):
                raise ValueError("Unsafe texture dimensions")
            indexed = np.frombuffer(self.data, dtype=np.uint8, count=width * height, offset=pixels)
            palette = np.frombuffer(self.data, dtype=np.uint8, count=768, offset=pixels + width * height).reshape(256, 3)
            rgba = np.empty((width * height, 4), dtype=np.uint8)
            rgba[:, :3] = palette[indexed]
            rgba[:, 3] = np.where((indexed == 255) & bool(flags & 64), 0, 255)
            textures.append((width, height, rgba.tobytes()))
        if not textures:
            raise ValueError("External texture models are not supported by this viewmodel exporter")
        skin = struct.unpack_from(f"<{self.integer(192)}h", self.data, self.integer(200))
        meshes = []
        for body in range(self.integer(204)):
            body_at = self.integer(208) + body * 76
            model = self.integer(body_at + 72)  # first body variation is the normal weapon
            vertex_count = self.integer(model + 80)
            vertices = self.floats(self.integer(model + 88), vertex_count * 3).reshape(-1, 3)
            bones = self.data[self.integer(model + 84):self.integer(model + 84) + vertex_count]
            for mesh in range(self.integer(model + 72)):
                mesh_at = self.integer(model + 76) + mesh * 20
                cursor = self.integer(mesh_at + 4)
                texture = skin[self.integer(mesh_at + 8)]
                # The owned M4/USP templates put the complete suppressor in a dedicated silencer texture mesh.
                # Omit that geometry while preserving the barrel, hands, rig and unsilenced animation timelines.
                if texture_names[texture].casefold() in excluded:
                    excluded_meshes += 1
                    continue
                width, height, _ = textures[texture]
                packed = bytearray()
                while True:
                    count = struct.unpack_from("<h", self.data, cursor)[0]
                    cursor += 2
                    if not count:
                        break
                    indices = [struct.unpack_from("<4h", self.data, cursor + i * 8) for i in range(abs(count))]
                    cursor += abs(count) * 8
                    # Triangle fans and alternating strips become an explicit triangle list.
                    for i in range(2, len(indices)):
                        triangle = (0, i - 1, i) if count < 0 else ((i - 2, i - 1, i) if i % 2 == 0 else (i - 1, i - 2, i))
                        for corner in triangle:
                            vertex, _, s, t = indices[corner]
                            packed.extend(struct.pack("<5fI", *vertices[vertex], s / width, t / height, bones[vertex]))
                meshes.append((texture, bytes(packed)))
        sequences = self.sequences()
        with destination.open("wb") as output:
            output.write(struct.pack("<4s4I", b"WCG2", len(textures), len(meshes), len(sequences), len(self.bones)))
            for width, height, rgba in textures:
                output.write(struct.pack("<2I", width, height) + rgba)
            for texture, packed in meshes:
                output.write(struct.pack("<2I", texture, len(packed) // 24) + packed)
            for name, fps, frames, matrices in sequences:
                output.write(struct.pack("<32sfI", name, fps, frames) + matrices)
        return {"model": self.path.name, "sha256": hashlib.sha256(self.data).hexdigest(),
                "textures": len(textures), "triangles": sum(len(m[1]) // 72 for m in meshes),
                "bones": len(self.bones), "sequences": [s[0].decode("ascii") for s in sequences],
                "cache_bytes": destination.stat().st_size,
                "excluded_textures": sorted(excluded), "excluded_meshes": excluded_meshes}

    def audio_timeline(self, destination: Path) -> set[str]:
        # Studio event 5004 supplies the original sound's animation frame, not a guess.
        sounds = set()
        with destination.open("wb") as output:
            count = self.integer(164)
            output.write(struct.pack("<4sI", b"WCA1", count))
            for i in range(count):
                at = self.integer(168) + i * 176
                fps = float(self.floats(at + 32, 1)[0])
                events = []
                for j in range(self.integer(at + 48)):
                    frame, code, _, option = struct.unpack_from("<iii64s", self.data, self.integer(at + 52) + j * 76)
                    if code == 5004:
                        name = Path(option.split(b"\0")[0].decode("ascii").replace("\\", "/")).name
                        sounds.add(name)
                        events.append((frame / fps, name))
                output.write(struct.pack("<32sfI", self.data[at:at + 32], (self.integer(at + 56) - 1) / fps, len(events)))
                for time, name in events:
                    output.write(struct.pack("<f64s", time, name.encode("ascii")))
        return sounds


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cstrike", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    # Prefer the owner's private Grudge sword even when invoking the converter outside setup.
    preferred_sword = Path(__file__).resolve().parents[1] / '.local/models/v_grudge_sword.mdl'
    if args.sword_model is None and preferred_sword.is_file():
        args.sword_model = preferred_sword
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = []
    weapon_sounds = set()
    for weapon in ("ak47", "m4a1", "usp", "awp", "knife", "c4"):
        model = StudioModel(args.cstrike / "models" / f"v_{weapon}.mdl")
        # Both silencer-capable weapons are permanently unsilenced in this mod, including their geometry.
        excluded = ("silencer.bmp",) if weapon in ("m4a1", "usp") else ()
        manifest.append(model.export(args.output / f"{weapon}.wcg", excluded))
        weapon_sounds.update(model.audio_timeline(args.output / f"{weapon}.wca"))
    # The release uses our original silver blade/gold guard; no downloaded sword model is imported.
    manifest.append(export_greatsword(args.output / "knife.wcg", args.output / "sword.wcg"))
    shutil.copy2(args.output / "knife.wca", args.output / "sword.wca")
    # Keep retail-derived files in the private lab, with their source fingerprints.
    sounds = args.output / "sounds"
    sounds.mkdir(exist_ok=True)
    weapon_sounds.update(("ak47-1.wav", "m4a1_unsil-1.wav", "usp_unsil-1.wav", "awp1.wav", "knife_slash1.wav", "knife_hit1.wav", "knife_hit2.wav", "knife_hit3.wav", "knife_hit4.wav", "knife_stab.wav", "knife_hitwall1.wav", "c4_plant.wav", "c4_click.wav",
                          "c4_explode1.wav", "c4_beep1.wav", "c4_beep3.wav", "c4_beep4.wav", "c4_beep5.wav"))
    for name in sorted(weapon_sounds):
        source = args.cstrike / "sound" / "weapons" / name
        shutil.copy2(source, sounds / name)
    # Default concrete samples cover footsteps, jump push-off and landings independently of weapons.
    for step in range(1, 5):
        shutil.copy2(args.cstrike / "sound" / "player" / f"pl_step{step}.wav", sounds / f"pl_step{step}.wav")
    (args.output / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
