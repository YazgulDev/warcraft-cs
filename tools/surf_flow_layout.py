"""A physically joined east/north/east course with flat turning/checkpoint decks."""
from dataclasses import dataclass
from math import ceil
from surf_course_layout import Pad

HALF_PAD, HALF_RAIL, BANK = 768, 384, 672
PIT = -2000
PADS = (Pad(-6400,-4096,1900,0), Pad(-2048,-4096,800,90),
        Pad(-2048,256,-300,0), Pad(4096,256,-1400,0))


@dataclass(frozen=True)
class Span:
    x: float
    y: float
    z: float
    length: float
    drop: float
    bank0: float
    bank1: float
    direction: int
    stage: int


def spans() -> tuple[Span, ...]:
    result=[]
    for stage,(start,end) in enumerate(zip(PADS,PADS[1:]),1):
        north=start.direction==90
        length=(end.y-start.y if north else end.x-start.x)-2*HALF_PAD
        count=ceil(length/256)
        # Shared cross-sections are the seam contract: bank height reaches zero at both
        # decks, so no launch gap, teleport or unmatched vertical lip is required.
        # Fade over the remaining three quarters: its outer-edge rise stays below
        # the ridge's descent. A short final fade would create an uphill speed trap.
        def bank(t):return BANK*min(t*4,(1-t)/.75)
        for index in range(count):
            a,b=index/count,(index+1)/count
            distance=HALF_PAD+length*(a+b)/2
            result.append(Span(start.x+(0 if north else distance),start.y+(distance if north else 0),
                               start.z+(end.z-start.z)*(a+b)/2,length/count,
                               (end.z-start.z)*(b-a),bank(a),bank(b),start.direction,stage))
    return tuple(result)


def script_data() -> str:
    lines=['    set courseTitle="SURF FLOW"',
           '    set courseHelp="Все рампы соединены с площадками. Синий: восток, оранжевый: север, финальный: восток. На площадках тормози и поворачивай; -cp нужен только при падении."',
           '    set courseStartEdge=768.', '    set courseStartMinimum=900.',
           '    set courseFall=-1850.', '    set coursePadWidth=620.',
           '    set coursePadDepth=620.', '    set courseFinishWidth=620.']
    for i,pad in enumerate(PADS):
        for key,value in (('cpX',pad.x),('cpY',pad.y),('cpZ',pad.z),('cpFacing',pad.direction)):
            lines.append(f'    set {key}[{i}]={value:.2f}')
        raw='FP02' if i==3 else 'FP01'
        lines.append(f"    call SkylineSolid('{raw}',{pad.x:.2f},{pad.y:.2f},{pad.z:.2f},0.)")
    for i,span in enumerate(spans()):
        lines.append(f"    call SkylineSolid('SF{i:02}',{span.x:.2f},{span.y:.2f},{span.z:.2f},{span.direction:.2f})")
    return '\n'.join(lines)
