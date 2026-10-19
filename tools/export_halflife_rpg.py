"""Optional RPG conversion from the owner's sibling valve directory or its PAK files."""
from __future__ import annotations
import json
import io
import wave
import shutil
import struct
import tempfile
from pathlib import Path

SOURCES = ('models/v_rpg.mdl', 'models/rpgrocket.mdl', 'sound/weapons/rocketfire1.wav',
           'sound/weapons/rocket1.wav', 'sound/weapons/debris1.wav')
OUTPUTS = ('rpg.wcg', 'rpg.wca', 'buy/rpg.wcg', 'world/rpgrocket.wcg',
           'sounds/hl_rpg_fire.wav', 'sounds/hl_rpg_motor.wav', 'sounds/hl_rpg_explode.wav')


class OwnedFiles:
    def __init__(self, root: Path):
        self.root = root

    def read(self, name: str) -> bytes | None:
        # Loose files override stock PAK data, matching an installed GoldSrc content directory.
        file = self.root / name
        if file.is_file():
            return file.read_bytes()
        for pak in sorted(self.root.glob('pak*.pak'), reverse=True):
            with pak.open('rb') as source:
                header = source.read(12)
                if len(header) != 12 or header[:4] != b'PACK':
                    continue
                offset, size = struct.unpack_from('<II', header, 4)
                length = pak.stat().st_size
                if size % 64 or size > 64 * 65536 or offset + size > length:
                    raise ValueError('Invalid owned PAK directory')
                source.seek(offset)
                directory = source.read(size)
                for at in range(0, size, 64):
                    entry = directory[at:at+56].split(b'\0')[0].decode('ascii').replace('\\', '/')
                    if entry.casefold() != name.casefold():
                        continue
                    start, count = struct.unpack_from('<II', directory, at+56)
                    if count > 64 * 1024 * 1024 or start + count > length:
                        raise ValueError('Invalid owned PAK entry')
                    source.seek(start)
                    data = source.read(count)
                    if len(data) != count:
                        raise ValueError('Truncated owned PAK entry')
                    return data
        return None


def fingerprint(data: bytes) -> int:
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return value


def export(cstrike: Path, output: Path) -> dict:
    from export_models import StudioModel
    from export_buy_previews import export_preview
    output.mkdir(parents=True, exist_ok=True)
    marker = output / 'rpg.ready'
    # A stale cache must never unlock RPG after updating from an incomplete/absent installation.
    marker.unlink(missing_ok=True)
    source = OwnedFiles(cstrike.parent / 'valve')
    try:
        data = {name: source.read(name) for name in SOURCES}
        if any(value is None for value in data.values()):
            return {'available': False, 'reason': 'Optional Half-Life RPG files were not found'}
        with tempfile.TemporaryDirectory(prefix='.rpg-', dir=output) as temporary:
            stage = Path(temporary)
            raw = stage / 'owned'
            raw.mkdir()
            for name in SOURCES[:2]:
                (raw / Path(name).name).write_bytes(data[name])
            held = StudioModel(raw / 'v_rpg.mdl')
            # External animation groups stay private and are resolved only by their local basename.
            for i in range(1, held.integer(172)):
                at = held.integer(176) + i*104
                name = Path(held.data[at+32:at+96].split(b'\0')[0].decode('ascii').replace('\\', '/')).name
                group = source.read('models/' + name)
                if group is None:
                    raise ValueError('Missing RPG animation group')
                (raw / name).write_bytes(group)
            held.export(stage / 'rpg.wcg', muzzle_attachments=())
            held.audio_timeline(stage / 'rpg.wca')
            export_preview(stage / 'rpg.wcg', stage / 'buy/rpg.wcg', held.texture_names(),
                           ('GLOVED_sleeve.bmp', 'rubbergloveCHROME.bmp', 'GLOVED_knuckle.bmp', 'GLOVE_handpak.bmp.bmp'))
            (stage / 'world').mkdir()
            StudioModel(raw / 'rpgrocket.mdl').export(stage / 'world/rpgrocket.wcg', muzzle_attachments=())
            (stage / 'sounds').mkdir()
            for source_name, target in zip(SOURCES[2:], OUTPUTS[4:]):
                sound = data[source_name]
                if len(sound) < 12 or sound[:4] != b'RIFF' or sound[8:12] != b'WAVE':
                    raise ValueError('Invalid RPG sound')
                with wave.open(io.BytesIO(sound), 'rb') as pcm:
                    frames = pcm.getnframes()
                    if pcm.getcomptype() != 'NONE' or not frames or len(pcm.readframes(frames)) != frames*pcm.getnchannels()*pcm.getsampwidth():
                        raise ValueError('Incomplete RPG PCM sound')
                (stage / target).write_bytes(sound)
            seal = bytearray(b'WRP1')
            for name in OUTPUTS:
                content = (stage / name).read_bytes()
                seal += struct.pack('<QQ', len(content), fingerprint(content))
                target = output / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(stage / name, target)
            # Publish the seal last. Partial conversion or modified/missing caches remain unavailable.
            pending = output / 'rpg.ready.tmp'
            pending.write_bytes(seal)
            pending.replace(marker)
        return {'available': True, 'weapon': 'Half-Life RPG', 'files': list(OUTPUTS)}
    except (OSError, ValueError, struct.error, IndexError, wave.Error, EOFError) as error:
        marker.unlink(missing_ok=True)
        return {'available': False, 'reason': 'Optional RPG conversion skipped: ' + str(error)}


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cstrike', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(export(args.cstrike, args.output), indent=2))
