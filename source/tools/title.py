import struct, sys
sys.path.insert(0, '.')
import mxl_d2ui_dc6 as DC6
GAMMA = 0.6
LUT = [min(255, round(255 * (v / 255) ** GAMMA)) for v in range(256)]

def font():
    tbl = open('fontexocet10.tbl', 'rb').read()
    frames = DC6.dc6_frames(open('fontexocet10.dc6', 'rb').read())
    g = {}
    for i in range((len(tbl) - 12) // 14):
        code, _u, w, h, _u2, _u3, fidx = struct.unpack_from('<HBBBBHH', tbl, 12 + i * 14)
        if fidx < len(frames):
            g[code] = (w, h, frames[fidx])
    return g

def text(s, color=(240, 200, 90)):
    glyphs = font()
    chars = [glyphs.get(ord(c)) or glyphs.get(ord('?')) for c in s]
    W = max(1, sum(g[0] for g in chars if g))
    top = max([g[2][1] - g[2][3] for g in chars if g] + [1])
    bottom = max([g[2][3] for g in chars if g] + [0])
    H = max(1, top + bottom)
    on0 = [0] * (W * H)
    x = 0
    for g in chars:
        adv, _h, fr = g
        fw, fh, ox, oy, px, mask = fr
        y0 = top - (fh - oy)
        for yy in range(fh):
            ty = y0 + yy
            if not 0 <= ty < H: continue
            for xx in range(fw):
                tx = x + ox + xx
                if mask[yy * fw + xx] and 0 <= tx < W:
                    on0[ty * W + tx] = 1
        x += adv
    W2, H2 = W + 2, H + 2
    on = [0 < x < W2 - 1 and 0 < y < H2 - 1 and on0[(y - 1) * W + x - 1] for y in range(H2) for x in range(W2)]
    out = bytearray(W2 * H2 * 4)
    for y in range(H2):
        for x in range(W2):
            j = (y * W2 + x) * 4
            if on[y * W2 + x]:
                out[j:j + 4] = bytes((LUT[color[0]], LUT[color[1]], LUT[color[2]], 255))
            elif any(0 <= x + dx < W2 and 0 <= y + dy < H2 and on[(y + dy) * W2 + x + dx] for dx in (-1, 0, 1) for dy in (-1, 0, 1)):
                out[j:j + 4] = bytes((LUT[12], LUT[8], LUT[4], 230))
    rows = [y for y in range(H2) if any(out[(y * W2 + xx) * 4 + 3] for xx in range(W2))]
    y0, y1 = rows[0], rows[-1] + 1
    return W2, y1 - y0, bytes(out[y0 * W2 * 4:y1 * W2 * 4])
