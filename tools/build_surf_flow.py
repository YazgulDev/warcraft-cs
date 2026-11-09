"""Build an original connected surf course without reading retail maps or art."""
import argparse
from pathlib import Path
import struct
from build_surf_map import build as practice_files
from build_surf_course import CELLS, ORIGIN, terrain, info, course_script
from map_archive import map_archive
from surf_flow_layout import PADS, PIT, HALF_PAD, HALF_RAIL, spans, script_data
from surf_flow_models import assets, objects

ROOT=Path(__file__).resolve().parents[1]
TITLE='Warcraft CS - Surf Flow'


def preview():
    size=256
    image=bytearray(struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,size,size,24,0x20))
    pieces=spans()
    for row in range(size):
        for col in range(size):
            x,y=ORIGIN+col*16384/size,-ORIGIN-row*16384/size
            rgb=(16,23,34)
            for span in pieces:
                along,across=(x-span.x,y-span.y) if span.direction==0 else (y-span.y,x-span.x)
                if abs(along)<=span.length/2 and abs(across)<=HALF_RAIL:
                    rgb=(240,145,50) if span.stage==2 else (60,155,240)
            for i,pad in enumerate(PADS):
                if abs(x-pad.x)<=HALF_PAD and abs(y-pad.y)<=HALF_PAD:rgb=(235,210,65) if i==3 else (60,210,125)
            image+=bytes(reversed(rgb))
    return bytes(image)


def build():
    _,files=practice_files((55,60,65),1600)
    side=CELLS*4
    art=assets()
    files.update({'war3map.w3i':info(TITLE,'Connected surf time trial. Joined ramps, broad turning decks, two checkpoints and session best.',PADS[0]),
                  'war3map.w3e':terrain(PIT),'war3map.wpm':b'MP3W'+struct.pack('<III',0,side,side)+bytes(side*side),
                  'war3map.shd':bytes(side*side),'war3map.w3b':objects()})
    files.update(art)
    files['war3map.j']=course_script(script_data(),TITLE,
        'Непрерывная surf-трасса: соединённые рампы, широкие площадки поворотов, 2 чекпоинта, таймер и рекорд сессии.',PADS[0],
        ('START -> EAST / F6 FPS','CP 1 -> NORTH / TURN LEFT','CP 2 -> EAST / TURN RIGHT','FINISH / -restart'))
    files['war3mapMap.tga']=files['war3mapPreview.tga']=preview()
    files['war3map.imp']=struct.pack('<II',1,len(art))+b''.join(b'\x0d'+name.replace('/','\\').encode()+b'\0' for name in art)
    return map_archive(files,TITLE),files


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'.local/surf-flow/SurfFlow.w3m')
    parser.add_argument('--script-output',type=Path)
    args=parser.parse_args();raw,files=build()
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(raw)
    if args.script_output:
        args.script_output.parent.mkdir(parents=True,exist_ok=True);args.script_output.write_bytes(files['war3map.j'])
    print(f'Built {args.output}: {len(raw)} bytes, {len(spans())} joined ramp pieces and four connected decks.')


if __name__=='__main__':main()
