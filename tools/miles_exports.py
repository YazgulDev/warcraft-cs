"""Generate exact sound-library export forwarders for the private runtime copy."""
from pathlib import Path
import struct
import sys


def generate(source: Path, destination: Path) -> None:
    data = source.read_bytes()
    u16 = lambda offset: struct.unpack_from('<H', data, offset)[0]
    u32 = lambda offset: struct.unpack_from('<I', data, offset)[0]
    pe = u32(0x3C)
    if data[pe:pe + 4] != b'PE\0\0' or u16(pe + 4) != 0x14C:
        raise ValueError('An x86 PE sound library is required')
    optional = pe + 24
    sections = optional + u16(pe + 20)

    def file_offset(rva: int) -> int:
        # Export pointers are RVAs, so map them through the original section table.
        for index in range(u16(pe + 6)):
            section = sections + index * 40
            address, size = u32(section + 12), max(u32(section + 8), u32(section + 16))
            if address <= rva < address + size:
                return u32(section + 20) + rva - address
        raise ValueError(f'Unmapped RVA {rva:x}')

    export = file_offset(u32(optional + 96))
    base, count, names_count = u32(export + 16), u32(export + 20), u32(export + 24)
    functions = file_offset(u32(export + 28))
    names = file_offset(u32(export + 32))
    ordinals = file_offset(u32(export + 36))
    labels = {}
    for index in range(names_count):
        name_offset = file_offset(u32(names + index * 4))
        name = data[name_offset:data.index(b'\0', name_offset)].decode('ascii')
        labels[u16(ordinals + index * 2)] = name
    lines = ['LIBRARY Mss32', 'EXPORTS']
    # Preserve both named and ordinal-only exports, without reimplementing sound behavior.
    for index in range(count):
        if not u32(functions + index * 4):
            continue
        ordinal = base + index
        name = labels.get(index)
        if name:
            lines.append(f'  {name}=WarcraftOriginalMss.{name} @{ordinal}')
        else:
            lines.append(f'  original_{ordinal}=WarcraftOriginalMss.#{ordinal} @{ordinal} NONAME')
    destination.write_text('\n'.join(lines) + '\n', encoding='ascii')
    # MSVC's import-library pass requires C anchors for undecorated forwarders.
    # The final exports still point to the original DLL, verified with dumpbin.
    anchors = ['// Linker anchors only: all public calls use PE export forwarders.']
    for name in labels.values():
        if not name.startswith(('_', '@')):
            anchors.append(f'extern "C" void {name}() {{}}')
    destination.with_suffix('.cpp').write_text('\n'.join(anchors) + '\n', encoding='ascii')
    print(f'Generated {len(lines) - 2} export forwarders')


if __name__ == '__main__':
    generate(Path(sys.argv[1]), Path(sys.argv[2]))
