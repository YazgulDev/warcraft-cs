"""Generate the project's original silver blade and gold guard on the owner's animated knife grip."""
import struct
from pathlib import Path


def export_greatsword(knife: Path, destination: Path):
    data = knife.read_bytes()
    _, texture_count, mesh_count, sequence_count, bones = struct.unpack_from('<4s4I', data)
    cursor, textures, meshes = 20, [], []
    for _ in range(texture_count):
        width, height = struct.unpack_from('<2I', data, cursor)
        length = 8 + width * height * 4
        textures.append(data[cursor:cursor + length]); cursor += length
    grip = None
    for _ in range(mesh_count):
        texture, count = struct.unpack_from('<2I', data, cursor)
        length = 8 + count * 24
        mesh = data[cursor:cursor + length]; cursor += length
        if texture == 0:
            # Retail knife geometry is attached to one grip bone; preserve that animated pivot.
            grip = struct.unpack_from('<I', mesh, 28)[0]
        else:
            meshes.append(mesh)
    if grip is None:
        raise ValueError('Knife blade/grip not found')
    generated = [[] for _ in range(5)]

    def triangle(material, a, b, c):
        for point in (a, b, c):
            generated[material].append(struct.pack('<5fI', *point, 0.5, 0.5, grip))

    def box(material, minimum, maximum):
        p = [(x, y, z) for z in (minimum[2], maximum[2])
             for y in (minimum[1], maximum[1]) for x in (minimum[0], maximum[0])]
        for a, b, c, d in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1),
                           (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
            triangle(material, p[a], p[b], p[c]); triangle(material, p[a], p[c], p[d])

    # A diamond blade gives two silver facets and a sharp tapered point without relying on lighting.
    rings = [[(-0.16, 0, z), (0, -w, z), (0.16, 0, z), (0, w, z)]
             for z, w in ((3.1, 1.15), (20, 0.9), (26, 0))]
    for lower, upper in zip(rings, rings[1:]):
        for side in range(4):
            nxt = (side + 1) % 4
            triangle(side % 2, lower[side], lower[nxt], upper[nxt])
            triangle(side % 2, lower[side], upper[nxt], upper[side])
    box(2, (-0.45, -3.0, 2.2), (0.45, 3.0, 3.1))
    box(3, (-0.35, -0.5, -3.5), (0.35, 0.5, 2.2))
    box(2, (-0.5, -0.65, -4.3), (0.5, 0.65, -3.5))
    # Contrasting leather bands remain visible at the grip and below the gauntlet.
    for z in (-3.1, -2.5, -1.9, -1.3, -0.7, -0.1, 0.5, 1.1, 1.7):
        box(4, (-0.37, -0.52, z), (0.37, 0.52, z + 0.12))
    colors = ((206, 218, 230), (122, 143, 164), (184, 146, 66), (53, 33, 24), (104, 68, 43))
    for index, color in enumerate(colors):
        textures.append(struct.pack('<2I4B', 1, 1, *color, 255))
        packed = b''.join(generated[index])
        meshes.append(struct.pack('<2I', texture_count + index, len(packed) // 24) + packed)
    destination.write_bytes(struct.pack('<4s4I', b'WCG2', len(textures), len(meshes), sequence_count, bones)
                            + b''.join(textures) + b''.join(meshes) + data[cursor:])
    return {'model': 'procedural_greatsword', 'source_hands': knife.name,
            'cache_bytes': destination.stat().st_size, 'blade_length': 22.9}
