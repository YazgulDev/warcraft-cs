"""Write small authored Warcraft maps without reading a retail map or game archive."""
from __future__ import annotations
import struct


def crypt_table() -> list[int]:
    table = [0] * 1280
    seed = 0x100001
    for column in range(256):
        for row in range(5):
            seed = (seed * 125 + 3) % 0x2AAAAB
            high = (seed & 65535) << 16
            seed = (seed * 125 + 3) % 0x2AAAAB
            table[row * 256 + column] = high | (seed & 65535)
    return table


TABLE = crypt_table()


def hash_name(name: str, kind: int) -> int:
    first, second = 0x7FED7FED, 0xEEEEEEEE
    for character in name.replace('/', '\\').upper().encode('ascii'):
        first = (TABLE[kind * 256 + character] ^ (first + second)) & 0xFFFFFFFF
        second = (character + first + second + (second << 5) + 3) & 0xFFFFFFFF
    return first


def encrypt_table(data: bytes, key: int) -> bytes:
    # MPQ protects its directory tables; file payloads below remain ordinary single-unit data.
    rolling = 0xEEEEEEEE
    result = bytearray()
    for (plain,) in struct.iter_unpack('<I', data):
        rolling = (rolling + TABLE[0x400 + (key & 255)]) & 0xFFFFFFFF
        result += struct.pack('<I', plain ^ ((key + rolling) & 0xFFFFFFFF))
        key = (((~key << 21) + 0x11111111) | (key >> 11)) & 0xFFFFFFFF
        rolling = (plain + rolling + (rolling << 5) + 3) & 0xFFFFFFFF
    return bytes(result)


def map_archive(files: dict[str, bytes], title: str) -> bytes:
    """Encode caller-authored files only, with a classic 512-byte map browser header."""
    files = dict(files)
    files['(listfile)'] = ('\r\n'.join((*files, '(listfile)')) + '\r\n').encode('ascii')
    count = 8
    while count < len(files) * 2:
        count *= 2
    hashes = [b'\xff' * 16 for _ in range(count)]
    blocks, payload = [], bytearray()
    names = set()
    for index, (name, data) in enumerate(files.items()):
        if name.lower() in names or any(p in ('', '..') for p in name.replace('\\', '/').split('/')):
            raise ValueError('Map filenames must be unique relative archive paths')
        names.add(name.lower())
        # Store directly for the classic Storm reader; an outer download ZIP handles compression.
        blocks.append(struct.pack('<IIII', 32 + len(payload), len(data), len(data), 0x81000000))
        payload += data
        slot = hash_name(name, 0) % count
        while hashes[slot] != b'\xff' * 16:
            slot = (slot + 1) % count
        hashes[slot] = struct.pack('<IIHHI', hash_name(name, 1), hash_name(name, 2), 0, 0, index)
    directory = encrypt_table(b''.join(hashes), hash_name('(hash table)', 3))
    blocks_data = encrypt_table(b''.join(blocks), hash_name('(block table)', 3))
    mpq = struct.pack('<4sIIHHIIII', b'MPQ\x1a', 32,
                      32 + len(payload) + len(directory) + len(blocks_data), 0, 3,
                      32 + len(payload), 32 + len(payload) + len(directory), count, len(files))
    browser = b'HM3W' + struct.pack('<I', 0) + title.encode('utf-8') + b'\0' + struct.pack('<II', 0x60, 1)
    if len(browser) > 512:
        raise ValueError('Map title exceeds the classic browser header')
    return browser.ljust(512, b'\0') + mpq + payload + directory + blocks_data
