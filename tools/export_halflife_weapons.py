"""Convert independently optional owned Half-Life weapons; public packages contain no retail content."""
from __future__ import annotations
import io
import shutil
import struct
import tempfile
import wave
from pathlib import Path
from export_halflife_rpg import OwnedFiles, fingerprint

WEAPONS = {
    'crowbar': ('cbar_miss1.wav', 'cbar_hit1.wav', 'cbar_hitbod1.wav', 'cbar_hit2.wav', 'cbar_hitbod2.wav', 'cbar_hitbod3.wav', 'cbar_ground1.wav', 'cbar_ground2.wav', 'cbar_ground3.wav'),
    'gauss': ('gauss2.wav', 'gauss_charge.wav'),
    'egon': ('egon_windup2.wav', 'egon_run3.wav', 'egon_off1.wav'),
}
GLOVES = ('GLOVED_sleeve.bmp', 'rubbergloveCHROME.bmp', 'GLOVED_knuckle.bmp', 'GLOVE_handpak.bmp.bmp')

def sound_source(name: str) -> str:
    # Soft ground uses the owner's dirt contacts, distinct from metal-bar and flesh impact variants.
    if name.startswith('cbar_ground'): return 'sound/player/pl_dirt' + name[len('cbar_ground'):]
    # The real Tau spinner is an ambience sample; store it under a unique weapon-cache alias.
    return 'sound/ambience/pulsemachine.wav' if name == 'gauss_charge.wav' else 'sound/weapons/' + name


def outputs(stem: str) -> tuple[str, ...]:
    return (f'{stem}.wcg', f'{stem}.wca', f'buy/{stem}.wcg') + tuple('sounds/hl_' + name for name in WEAPONS[stem])


def export_one(cstrike: Path, output: Path, stem: str) -> dict:
    from export_models import StudioModel
    from export_buy_previews import export_preview
    output.mkdir(parents=True, exist_ok=True)
    marker = output / f'{stem}.ready'
    marker.unlink(missing_ok=True)
    source = OwnedFiles(cstrike.parent / 'valve')
    try:
        model_bytes = source.read(f'models/v_{stem}.mdl')
        if model_bytes is None:
            return {'weapon': stem, 'available': False, 'reason': 'Owned Half-Life model absent'}
        with tempfile.TemporaryDirectory(prefix=f'.{stem}-', dir=output) as temporary:
            stage = Path(temporary)
            raw = stage / 'owned'
            raw.mkdir()
            model_path = raw / f'v_{stem}.mdl'
            model_path.write_bytes(model_bytes)
            model = StudioModel(model_path)
            # Animation groups are resolved locally by basename, never by an archive-supplied path.
            for i in range(1, model.integer(172)):
                at = model.integer(176) + i * 104
                name = Path(model.data[at+32:at+96].split(b'\0')[0].decode('ascii').replace('\\', '/')).name
                data = source.read('models/' + name)
                if data is None:
                    raise ValueError('Missing owned animation group')
                (raw / name).write_bytes(data)
            model.export(stage / f'{stem}.wcg', muzzle_attachments=(0,) if model.integer(212) else (), presentation=True,
                         muzzle_texture='nose_work.bmp' if stem == 'egon' else None)
            timeline = stage / f'{stem}.wca'
            samples = model.audio_timeline(timeline)
            # Prefix guest samples so an animation cannot collide with Counter-Strike's sound namespace.
            data = bytearray(timeline.read_bytes())
            count = struct.unpack_from('<I', data, 4)[0]
            at = 8
            for _ in range(count):
                events = struct.unpack_from('<I', data, at + 36)[0]
                at += 40
                for _ in range(events):
                    name = data[at+4:at+68].split(b'\0')[0].decode('ascii')
                    data[at+4:at+68] = struct.pack('<64s', ('hl_' + name).encode('ascii'))
                    at += 68
            timeline.write_bytes(data)
            export_preview(stage / f'{stem}.wcg', stage / f'buy/{stem}.wcg', model.texture_names(), GLOVES)
            sounds = stage / 'sounds'
            sounds.mkdir()
            for name in samples | set(WEAPONS[stem]):
                sound = source.read(sound_source(name))
                if sound is None and name not in WEAPONS[stem]:
                    continue
                if sound is None:
                    raise ValueError('Missing owned sound ' + name)
                with wave.open(io.BytesIO(sound), 'rb') as pcm:
                    frames = pcm.getnframes()
                    if pcm.getcomptype() != 'NONE' or not frames or len(pcm.readframes(frames)) != frames * pcm.getnchannels() * pcm.getsampwidth():
                        raise ValueError('Incomplete owned PCM sound')
                (sounds / ('hl_' + name)).write_bytes(sound)
            seal = bytearray(b'WHL1')
            for name in outputs(stem):
                content = (stage / name).read_bytes()
                seal += struct.pack('<QQ', len(content), fingerprint(content))
            for name in outputs(stem) + tuple(f'sounds/hl_{s}' for s in samples if (sounds / ('hl_' + s)).is_file()):
                target = output / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(stage / name, target)
            # Publish each seal last; missing or partial content stays unavailable without failing CS setup.
            pending = output / f'{stem}.ready.tmp'
            pending.write_bytes(seal)
            pending.replace(marker)
        return {'weapon': stem, 'available': True}
    except (OSError, ValueError, struct.error, IndexError, wave.Error, EOFError) as error:
        marker.unlink(missing_ok=True)
        return {'weapon': stem, 'available': False, 'reason': str(error)}


def export(cstrike: Path, output: Path) -> list[dict]:
    from export_energy_sprites import export as export_effects
    export_effects(cstrike, output)
    return [export_one(cstrike, output, stem) for stem in WEAPONS]
