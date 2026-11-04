"""Original low-poly MDX 800 surf geometry and checker textures; no game assets are read."""
from __future__ import annotations
import math
import struct


def chunk(tag: bytes, data: bytes) -> bytes:
    return tag + struct.pack('<I', len(data)) + data


def extent(points: list[tuple[float, float, float]]) -> bytes:
    low = [min(p[i] for p in points) for i in range(3)]
    high = [max(p[i] for p in points) for i in range(3)]
    radius = max(math.sqrt(sum(v*v for v in p)) for p in points)
    return struct.pack('<7f', radius, *low, *high)


def model(triangles: list[tuple], texture: str, title: str) -> bytes:
    # A single stationary root bone and opaque Stand geoset give the engine and collision
    # decoder the same vertices. Extents include the real solid, not an oversized helper.
    points = [p for triangle in triangles for p in triangle]
    bounds = extent(points)
    body = title.encode('ascii').ljust(80, b'\0') + bytes(260) + bounds + struct.pack('<I', 0)
    sequence = b'Stand'.ljust(80, b'\0') + struct.pack('<IIfIfI', 0, 1000, 0., 0, 0., 0) + bounds
    layer = struct.pack('<IIIiiIf', 28, 0, 1, 0, -1, 0, 1.)
    material = struct.pack('<III4sI', 48, 0, 0, b'LAYS', 1) + layer
    tex = struct.pack('<I', 0) + texture.encode('ascii').ljust(260, b'\0') + struct.pack('<I', 3)
    vertices, normals, uv = bytearray(), bytearray(), bytearray()
    for triangle in triangles:
        a = [triangle[1][i]-triangle[0][i] for i in range(3)]
        b = [triangle[2][i]-triangle[0][i] for i in range(3)]
        n = (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
        length = math.sqrt(sum(v*v for v in n))
        if length <= .001:
            raise ValueError('Degenerate ramp face')
        for point in triangle:
            vertices += struct.pack('<3f', *point)
            normals += struct.pack('<3f', *(v/length for v in n))
            uv += struct.pack('<2f', point[0]/128, (point[1]+point[2])/128)
    count = len(points)
    def array(tag, size, data):
        return tag + struct.pack('<I', size) + data
    geo = array(b'VRTX', count, vertices) + array(b'NRMS', count, normals)
    geo += array(b'PTYP', 1, struct.pack('<I', 4)) + array(b'PCNT', 1, struct.pack('<I', count))
    geo += array(b'PVTX', count, struct.pack('<'+'H'*count, *range(count)))
    geo += array(b'GNDX', count, bytes(count)) + array(b'MTGC', 1, struct.pack('<I', 1))
    geo += array(b'MATS', 1, struct.pack('<I', 0)) + struct.pack('<III', 0, 0, 0)
    geo += bounds + struct.pack('<I', 1) + bounds + b'UVAS' + struct.pack('<I', 1) + array(b'UVBS', count, uv)
    geo = struct.pack('<I', len(geo)+4) + geo
    node = struct.pack('<I', 96) + b'Root'.ljust(80, b'\0') + struct.pack('<IiI', 0, -1, 256)
    bone = node + struct.pack('<ii', 0, -1)
    return (b'MDLX' + chunk(b'VERS', struct.pack('<I', 800)) + chunk(b'MODL', body)
            + chunk(b'SEQS', sequence) + chunk(b'MTLS', material) + chunk(b'TEXS', tex)
            + chunk(b'GEOS', geo) + chunk(b'BONE', bone) + chunk(b'PIVT', bytes(12)))


def prism(length: float, section: list[tuple[float, float]], decline: float = 0) -> list[tuple]:
    # Convex Y/Z cross-sections are extruded along X, with outward winding on all faces.
    sides = [[(x, y, z-decline*x) for y, z in section] for x in (-length/2, length/2)]
    faces = []
    for i in range(1, len(section)-1):
        faces.append((sides[0][0], sides[0][i+1], sides[0][i]))
        faces.append((sides[1][0], sides[1][i], sides[1][i+1]))
    for i in range(len(section)):
        j = (i+1) % len(section)
        faces.extend(((sides[0][i], sides[0][j], sides[1][j]),
                      (sides[0][i], sides[1][j], sides[1][i])))
    return faces


def checker(color: tuple[int, int, int]) -> bytes:
    size = 64
    result = bytearray(struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, size, size, 24, 0x20))
    for y in range(size):
        for x in range(size):
            grid = x % 32 < 2 or y % 32 < 2
            scale = .35 if grid else 1 if (x//32+y//32) % 2 else .78
            result += bytes(int(c*scale) for c in reversed(color))
    return bytes(result)


def assets() -> dict[str, bytes]:
    prefix = 'WarcraftCS/Surf/'
    result = {}
    for name, color in (('Blue', (70, 160, 235)), ('Orange', (240, 145, 55)),
                        ('Green', (70, 210, 130)), ('Finish', (215, 195, 90))):
        result[prefix+name+'.tga'] = checker(color)
    ramps = prism(512, [(-256,-32), (256,-32), (256,0), (0,448), (-256,0)], .10)
    pad = prism(512, [(-448,-96), (448,-96), (448,0), (-448,0)])
    # A long finish catches fast gap exits and leaves room to brake before the far edge.
    finish = prism(1536, [(-448,-96), (448,-96), (448,0), (-448,0)])
    for name, mesh, texture in (('Rail', ramps, 'Blue'), ('RailOrange', ramps, 'Orange'),
                               ('Pad', pad, 'Green'), ('FinishPad', finish, 'Finish')):
        result[prefix+name+'.mdx'] = model(mesh, (prefix+texture+'.tga').replace('/', '\\'), 'Surf '+name)
    return result


def destructables() -> bytes:
    # Custom W3B fields reference only our imported models. Empty pathing leaves the precise
    # model collision in charge; the native walkable flag supplies initial placement height.
    result = bytearray(struct.pack('<III', 2, 0, 4))
    for raw, model_name in (('SR01','Rail'), ('SR02','RailOrange'), ('SP01','Pad'), ('SP02','FinishPad')):
        fields = [('bfil',3,'WarcraftCS\\Surf\\'+model_name+'.mdx'), ('bnam',3,'Surf '+model_name),
                  ('bptx',3,''), ('bvar',0,1), ('bwal',0,1), ('buch',0,0),
                  ('bhps',2,99999.), ('bmap',2,0.), ('bmar',2,0.)]
        result += b'LTba' + raw.encode('ascii') + struct.pack('<I', len(fields))
        for field, kind, value in fields:
            result += field.encode('ascii') + struct.pack('<I', kind)
            result += value.encode('ascii')+b'\0' if kind == 3 else struct.pack('<i' if kind == 0 else '<f', value)
            result += raw.encode('ascii')
    return bytes(result)
