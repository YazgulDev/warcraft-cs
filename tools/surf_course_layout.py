"""Authored Surf Skyline route: solid rails, gaps, checkpoint pads and a final landing."""
from __future__ import annotations
from dataclasses import dataclass


@dataclass(frozen=True)
class Pad:
    x: float
    y: float
    z: float
    direction: int


@dataclass(frozen=True)
class Rail:
    x: float
    y: float
    z: float
    direction: int
    stage: int


PADS = (Pad(-6400,-2600,1600,0), Pad(-2560,-2600,700,90),
        Pad(-2560,1472,150,0), Pad(1856,1472,-850,0))


def rails() -> tuple[Rail, ...]:
    # Stage 1 teaches side contact; stage 2 turns north; stage 3 adds two genuine air gaps.
    result = [Rail(-5824+i*512,-2600,1000-i*51.2,0,1) for i in range(6)]
    result += [Rail(-2560,-1856+i*512,400-i*51.2,90,2) for i in range(6)]
    heights=(-200,-251.2,-456,-507.2,-712,-763.2)
    result += [Rail(x,1472,z,0,3) for x,z in zip((-1856,-1344,-640,-128,576,1088),heights)]
    return tuple(result)


def script_data() -> str:
    lines = []
    for index, pad in enumerate(PADS):
        for key, value in (('cpX',pad.x),('cpY',pad.y),('cpZ',pad.z),('cpFacing',pad.direction)):
            lines.append(f'    set {key}[{index}]={float(value):.2f}')
        raw = 'SP02' if index == 3 else 'SP01'
        lines.append(f"    call SkylineSolid('{raw}',{pad.x:.2f},{pad.y:.2f},{pad.z:.2f},0.)")
    for rail in rails():
        raw = 'SR02' if rail.stage == 2 else 'SR01'
        lines.append(f"    call SkylineSolid('{raw}',{rail.x:.2f},{rail.y:.2f},{rail.z:.2f},{float(rail.direction):.2f})")
    return '\n'.join(lines)


def stage_text(index: int) -> str:
    return ('BLUE / EAST', 'ORANGE / NORTH', 'GAPS / EAST', 'FINISH')[index]
