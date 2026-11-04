"""Build the original Surf Skyline time-trial map, including its own MDX/TGA art."""
from __future__ import annotations
import argparse
from pathlib import Path
import struct
from build_surf_map import build as practice_files, text
from map_archive import map_archive
from surf_models import assets, destructables
from surf_course_layout import PADS, rails, script_data

ROOT = Path(__file__).resolve().parents[1]
CELLS, ORIGIN, PIT = 128, -8192, -1800


def terrain() -> bytes:
    # A low, flat safety floor never supplies hidden support beneath floating rails.
    data = bytearray(b'W3E!' + struct.pack('<IcII',11,b'L',1,1) + b'Ldrt')
    data += struct.pack('<I',2) + b'CLdiCLgr' + struct.pack('<IIff',CELLS+1,CELLS+1,ORIGIN,ORIGIN)
    data += struct.pack('<HHBBB',8192+PIT*4,0,0,0,2)*((CELLS+1)**2)
    return bytes(data)


def info() -> bytes:
    bounds=(-7808.,-7808.,7808.,7808.,-7808.,7808.,7808.,-7808.)
    data=struct.pack('<III',18,1,6052)+text('Warcraft CS - Surf Skyline')+text('Yazgul / Codex')
    data+=text('Original three-stage surf time trial. Floating rails, gaps, checkpoints. Requires the model-surf launcher.')+text('1 player')
    data+=struct.pack('<8f4i3I',*bounds,3,3,3,3,CELLS-6,CELLS-6,0x60)+b'L'
    data+=struct.pack('<i',-1)+text('F6: FPS. P: speed. -cp: checkpoint. -restart: full timed run. -stage 1/2/3: practice.')
    data+=text('SURF SKYLINE')+text('Three stages / two checkpoints')+struct.pack('<i',-1)+text('')*3
    data+=struct.pack('<5I',1,0,1,1,1)+text('Surfer')
    data+=struct.pack('<ffII',PADS[0].x,PADS[0].y-100,0,0)
    data+=struct.pack('<III',1,1,1)+text('Surf Skyline')+struct.pack('<III',0,0,0)
    return data


def preview() -> bytes:
    # Procedural overview follows the actual model transforms, including the north-facing stage.
    size=256
    image=bytearray(struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,size,size,24,0x20))
    for row in range(size):
        for column in range(size):
            x,y=ORIGIN+column*16384/size,-ORIGIN-row*16384/size
            rgb=(17,23,32)
            for rail in rails():
                dx,dy=x-rail.x,y-rail.y
                along,across=(dx,dy) if rail.direction==0 else (dy,-dx)
                if abs(along)<256 and abs(across)<256:
                    rgb=(235,145,55) if rail.stage==2 else (55,155,235)
            for index,pad in enumerate(PADS):
                # The final braking deck is longer than the checkpoint pads.
                if abs(x-pad.x)<(768 if index==3 else 256) and abs(y-pad.y)<448:rgb=(225,205,80) if index==3 else (75,220,140)
            image+=bytes(reversed(rgb))
    return bytes(image)


def build() -> tuple[bytes,dict[str,bytes]]:
    # Reuse our own empty placement/container records, never a retail map or extracted art.
    _, files=practice_files((55,60,65),1600)
    side=CELLS*4
    files.update({'war3map.w3i':info(),'war3map.w3e':terrain(),
                  'war3map.wpm':b'MP3W'+struct.pack('<III',0,side,side)+bytes(side*side),
                  'war3map.shd':bytes(side*side),'war3map.w3b':destructables()})
    files.update(assets())
    script=(ROOT/'tools/maps/SurfSkyline.j').read_text(encoding='utf-8').replace('@COURSE_DATA@',script_data())
    files['war3map.j']=script.encode('utf-8')
    files['war3mapMap.tga']=files['war3mapPreview.tga']=preview()
    # Import names are authored paths; models and textures are portable original map content.
    imported=tuple(assets())
    files['war3map.imp']=struct.pack('<II',1,len(imported))+b''.join(b'\x0d'+p.replace('/', '\\').encode('ascii')+b'\0' for p in imported)
    return map_archive(files,'Warcraft CS - Surf Skyline'),files


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'.local/surf-course/SurfSkyline.w3m')
    parser.add_argument('--script-output',type=Path)
    args=parser.parse_args();data,files=build()
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    if args.script_output:
        args.script_output.parent.mkdir(parents=True,exist_ok=True);args.script_output.write_bytes(files['war3map.j'])
    print(f'Built {args.output}: {len(data)} bytes, {len(rails())} rails, {len(PADS)-2} checkpoints, original imported models/textures.')


if __name__=='__main__':main()
