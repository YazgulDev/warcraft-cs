"""Render a private nuclear blast from the owner's CS/HL PCM samples; ship no retail audio."""
from __future__ import annotations
import configparser
import hashlib
import io
import json
import math
from pathlib import Path
import struct
import wave
from export_halflife_rpg import OwnedFiles

RATE = 44100
# These authored mix controls apply during private conversion, not to an already rendered WAV on F8.
TUNING = {
    'C4Volume': (.85, 0, 1), 'C4Pitch': (.85, .5, 1.5),
    'MortarVolume': (.7, 0, 1), 'MortarPitch': (.72, .5, 1.5), 'MortarDelaySeconds': (.08, 0, 2),
    'RumbleVolume': (.45, 0, 1), 'RumblePitch': (.7, .5, 1.5), 'RumbleDelaySeconds': (.45, 0, 4),
    'RumbleSeconds': (8, 0, 15), 'RumbleFadeSeconds': (3, .05, 10),
    'DebrisVolume': (.18, 0, 1), 'DebrisPitch': (.85, .5, 1.5), 'DebrisDelaySeconds': (1.4, 0, 5),
}
SOURCES = {
    'C4': ('cstrike', 'sound/weapons/c4_explode1.wav'),
    'Mortar': ('valve', 'sound/weapons/mortarhit.wav'),
    'Rumble': ('valve', 'sound/ambience/rocketrumble1.wav'),
    'Debris': ('valve', 'sound/weapons/debris3.wav'),
}


def settings(path: Path | None) -> dict:
    ini = configparser.ConfigParser(interpolation=None, strict=False)
    if path and path.is_file():
        try:
            ini.read_string(path.read_text(encoding='utf-8-sig'))
        except (configparser.Error, UnicodeError):
            ini.clear()
    section = next((name for name in ini.sections() if name.casefold() == "nuclearaudio"), "NuclearAudio")
    values = {}
    for key, (default, minimum, maximum) in TUNING.items():
        try:
            value = float(ini.get(section, key, fallback=str(default)))
            if not math.isfinite(value):
                value = default
        except ValueError:
            value = default
        values[key] = min(maximum, max(minimum, value))
    return values


def decode(data: bytes) -> tuple[list[float], int]:
    if len(data) > 16 * 1024 * 1024:
        raise ValueError('Owned nuclear sample is too large')
    with wave.open(io.BytesIO(data), 'rb') as audio:
        channels, width, rate, frames = audio.getnchannels(), audio.getsampwidth(), audio.getframerate(), audio.getnframes()
        if audio.getcomptype() != 'NONE' or channels not in (1, 2) or width not in (1, 2, 3, 4) or not 8000 <= rate <= 192000 or not 0 < frames <= rate * 20:
            raise ValueError('Unsupported nuclear PCM format/duration')
        raw = audio.readframes(frames)
        if len(raw) != frames * channels * width:
            raise ValueError('Incomplete nuclear PCM sample')
    samples = []
    divisor = float(1 << (width * 8 - 1))
    for frame in range(frames):
        value = 0.0
        for channel in range(channels):
            at = (frame * channels + channel) * width
            value += (raw[at] - 128) / 128 if width == 1 else int.from_bytes(raw[at:at + width], 'little', signed=True) / divisor
        samples.append(value / channels)
    return samples, rate


def resample(samples: list[float], rate: int, pitch: float) -> list[float]:
    # A slower sample clock lowers both pitch and speed without requiring ffmpeg or an audio SDK.
    step = rate * pitch / RATE
    count = max(1, math.ceil((len(samples) - 1) / step) + 1)
    result = []
    for i in range(count):
        position = min(len(samples) - 1, i * step)
        at = int(position); blend = position - at
        result.append(samples[at] * (1 - blend) + samples[min(at + 1, len(samples) - 1)] * blend)
    return result


def rumble(samples: list[float], seconds: float, fade: float) -> list[float]:
    count = int(seconds * RATE)
    cross = max(1, min(int(.08 * RATE), len(samples) // 4))
    period = max(1, len(samples) - cross)
    fade_frames = max(1, int(min(fade, seconds) * RATE))
    result = []
    for i in range(count):
        at = i % period; value = samples[at]
        if i >= period and at < cross:
            blend = at / cross
            value = samples[period + at] * (1 - blend) + value * blend
        # Declick loop boundaries, ease in after the blast and finish at exact silence.
        envelope = min(1, i / (RATE * .25), (count - 1 - i) / fade_frames)
        result.append(value * max(0, envelope))
    return result


def compose(inputs: dict[str, bytes], tuning: dict) -> tuple[bytes, dict]:
    layers = []
    for key in SOURCES:
        samples, rate = decode(inputs[key])
        samples = resample(samples, rate, tuning[key + 'Pitch'])
        if key == 'Rumble':
            samples = rumble(samples, tuning['RumbleSeconds'], tuning['RumbleFadeSeconds'])
        delay = 0 if key == 'C4' else tuning[key + 'DelaySeconds']
        layers.append((int(delay * RATE), samples, tuning[key + 'Volume']))
    count = max(start + len(samples) for start, samples, _ in layers)
    mixed = [0.0] * count
    for start, samples, gain in layers:
        for i, value in enumerate(samples):
            mixed[start + i] += value * gain
    peak = max((abs(value) for value in mixed), default=0)
    # Only attenuate excessive peaks; reducing a user layer's gain must never normalize it back up.
    gain = min(1, .92 / peak) if peak else 1
    pcm = bytearray(count * 2)
    for i, value in enumerate(mixed):
        struct.pack_into('<h', pcm, i * 2, round(max(-1, min(1, value * gain)) * 32767))
    stream = io.BytesIO()
    with wave.open(stream, 'wb') as audio:
        audio.setparams((1, 2, RATE, count, 'NONE', 'not compressed')); audio.writeframes(pcm)
    return stream.getvalue(), {'seconds': count / RATE, 'peak_before': peak, 'normalization_gain': gain, 'peak_after': peak * gain, 'settings': tuning}


def export(cstrike: Path, output: Path, config: Path | None = None) -> dict:
    cstrike, output = cstrike.resolve(), output.resolve()
    if output.is_relative_to(cstrike) or output.is_relative_to(cstrike.parent / 'valve'):
        raise ValueError('Generate nuclear audio in a separate private client directory')
    sounds = output / 'sounds'; sounds.mkdir(parents=True, exist_ok=True)
    target, pending = sounds / 'nuclear_blast.wav', sounds / 'nuclear_blast.wav.tmp'
    metadata = output / 'nuclear-audio.json'
    try:
        owners = {'cstrike': OwnedFiles(cstrike), 'valve': OwnedFiles(cstrike.parent / 'valve')}
        inputs, fingerprints = {}, {}
        for key, (game, path) in SOURCES.items():
            data = owners[game].read(path)
            if not data:
                raise ValueError('Missing optional owned nuclear sample: ' + game + '/' + path)
            inputs[key] = data; fingerprints[key] = {'game': game, 'path': path, 'sha256': hashlib.sha256(data).hexdigest()}
        data, report = compose(inputs, settings(config))
        # Publish the complete private WAV atomically, never a partial file or retail audio in the public archive.
        pending.write_bytes(data); pending.replace(target)
        report.update({'available': True, 'file': 'sounds/nuclear_blast.wav', 'sources': fingerprints, 'sha256': hashlib.sha256(data).hexdigest()})
        metadata.write_text(json.dumps(report, indent=2), encoding='utf-8')
        return report
    except (OSError, ValueError, wave.Error, EOFError, struct.error) as error:
        for generated in (pending, target, metadata):
            generated.unlink(missing_ok=True)
        return {'available': False, 'reason': str(error)}


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cstrike', type=Path, required=True); parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--config', type=Path)
    args = parser.parse_args(); print(json.dumps(export(args.cstrike, args.output, args.config), indent=2))
