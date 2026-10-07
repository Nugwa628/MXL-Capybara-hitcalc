"""Minimal MPQ v1 reader (D2): hash/block tables, sectors, zlib / bzip2 / PKWARE implode."""
import bz2
import struct
import zlib

_CT = []


def _init():
    seed = 0x00100001
    t = [0] * 0x500
    for i in range(0x100):
        j = i
        for _ in range(5):
            seed = (seed * 125 + 3) % 0x2AAAAB
            a = (seed & 0xFFFF) << 16
            seed = (seed * 125 + 3) % 0x2AAAAB
            t[j] = a | (seed & 0xFFFF)
            j += 0x100
    _CT.extend(t)


_init()


def hstr(s, kind):
    s1, s2 = 0x7FED7FED, 0xEEEEEEEE
    for c in s.upper().encode('latin-1'):
        s1 = (_CT[(kind << 8) + c] ^ (s1 + s2)) & 0xFFFFFFFF
        s2 = (c + s1 + s2 + (s2 << 5) + 3) & 0xFFFFFFFF
    return s1


def decrypt(data, key):
    n = len(data) // 4
    v = list(struct.unpack_from(f'<{n}I', data))
    s2 = 0xEEEEEEEE
    for i in range(n):
        s2 = (s2 + _CT[0x400 + (key & 0xFF)]) & 0xFFFFFFFF
        ch = v[i] ^ ((key + s2) & 0xFFFFFFFF)
        key = ((((~key) << 0x15) + 0x11111111) | (key >> 0x0B)) & 0xFFFFFFFF
        s2 = (ch + s2 + (s2 << 5) + 3) & 0xFFFFFFFF
        v[i] = ch
    return struct.pack(f'<{n}I', *v) + data[n * 4:]


# ---- PKWARE DCL explode (port of zlib's blast.c)
class _Huff:
    def __init__(self, rep):
        lens = []
        for b in rep:
            lens += [b & 15] * ((b >> 4) + 1)
        self.count = [0] * 16
        for l in lens:
            self.count[l] += 1
        offs = [0] * 16
        for l in range(1, 14):
            offs[l + 1] = offs[l] + self.count[l]
        self.sym = [0] * len(lens)
        for s, l in enumerate(lens):
            self.sym[offs[l]] = s
            offs[l] += 1


_LIT = _Huff(bytes([11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8, 9, 7, 6, 7, 8, 7, 6,
                    55, 8, 23, 24, 12, 11, 7, 9, 11, 12, 6, 7, 22, 5, 7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8,
                    25, 11, 8, 11, 9, 12, 8, 12, 5, 38, 5, 38, 5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27, 44,
                    253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12, 45, 44, 173]))
_LEN = _Huff(bytes([2, 35, 36, 53, 38, 23]))
_DIST = _Huff(bytes([2, 20, 53, 230, 247, 151, 248]))
_BASE = [3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264]
_EXTRA = [0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8]


def explode(src):
    st = {'pos': 0, 'buf': 0, 'cnt': 0}

    def bits(n):
        v, c = st['buf'], st['cnt']
        while c < n:
            v |= src[st['pos']] << c
            st['pos'] += 1
            c += 8
        st['buf'], st['cnt'] = v >> n, c - n
        return v & ((1 << n) - 1)

    def decode(h):
        code = first = index = 0
        bitbuf, left = st['buf'], st['cnt']
        ln = 1
        while True:
            while left:
                code |= (bitbuf & 1) ^ 1
                bitbuf >>= 1
                count = h.count[ln]
                if code < first + count:
                    st['buf'], st['cnt'] = bitbuf, (st['cnt'] - ln) & 7
                    return h.sym[index + (code - first)]
                index += count
                first += count
                first <<= 1
                code <<= 1
                ln += 1
                left -= 1
            left = 13 - ln
            bitbuf = src[st['pos']]
            st['pos'] += 1
            if left > 8:
                left = 8

    lit = bits(8)
    dictb = bits(8)
    out = bytearray()
    while True:
        if bits(1):
            sym = decode(_LEN)
            ln = _BASE[sym] + bits(_EXTRA[sym])
            if ln == 519:
                break
            sym = 2 if ln == 2 else dictb
            dist = (decode(_DIST) << sym) + bits(sym) + 1
            for _ in range(ln):
                out.append(out[-dist])
        else:
            out.append(decode(_LIT) if lit else bits(8))
    return bytes(out)


def _decomp(d):
    m = d[0]
    d = d[1:]
    if m & 0x10:
        d = bz2.decompress(d)
    if m & 0x08:
        d = explode(d)
    if m & 0x02:
        d = zlib.decompress(d)
    return d


class MPQ:
    def __init__(self, path):
        self.f = open(path, 'rb').read()
        o = self.f.find(b'MPQ\x1a')
        self.o = o
        _m, hs, _arc, _ver, sshift, hpos, bpos, hn, bn = struct.unpack_from('<4sIIHHIIII', self.f, o)
        self.sector = 512 << sshift
        self.ht = decrypt(self.f[o + hpos:o + hpos + hn * 16], hstr('(hash table)', 3))
        self.bt = decrypt(self.f[o + bpos:o + bpos + bn * 16], hstr('(block table)', 3))
        self.hn = hn

    def find(self, name):
        i = hstr(name, 0) % self.hn
        a, b = hstr(name, 1), hstr(name, 2)
        for _ in range(self.hn):
            ha, hb, _loc, blk = struct.unpack_from('<IIII', self.ht, i * 16)
            if blk == 0xFFFFFFFF:
                return None
            if ha == a and hb == b and blk != 0xFFFFFFFE:
                return blk
            i = (i + 1) % self.hn
        return None

    def read(self, name):
        blk = self.find(name)
        if blk is None:
            raise KeyError(name)
        pos, csize, fsize, flags = struct.unpack_from('<IIII', self.bt, blk * 16)
        data = self.f[self.o + pos:self.o + pos + csize]
        key = None
        if flags & 0x10000:
            key = hstr(name.replace('/', '\\').split('\\')[-1], 3)
            if flags & 0x20000:
                key = ((key + pos) ^ fsize) & 0xFFFFFFFF
        if flags & 0x1000000:                                     # single unit
            if key is not None:
                data = decrypt(data, key)
            return _decomp(data) if csize < fsize else data
        if not flags & 0x300:
            return decrypt(data, key) if key is not None else data
        ns = (fsize + self.sector - 1) // self.sector
        tab = data[:(ns + 1) * 4]
        if key is not None:
            tab = decrypt(tab, (key - 1) & 0xFFFFFFFF)
        offs = struct.unpack_from(f'<{ns + 1}I', tab)
        out = bytearray()
        for s in range(ns):
            chunk = data[offs[s]:offs[s + 1]]
            if key is not None:
                chunk = decrypt(chunk, (key + s) & 0xFFFFFFFF)
            want = min(self.sector, fsize - s * self.sector)
            if len(chunk) < want:
                if flags & 0x100:
                    chunk = explode(chunk)
                else:
                    chunk = _decomp(chunk)
            out += chunk
        return bytes(out)
