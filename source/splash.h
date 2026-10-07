/*
 * Loading screen, same as the MXL Capybara Editor's: Diablo II's door opening, the capybara on the platform, the
 * title on the plate in fontexocet10 gold. Everything is inside the exe (splash_data.h); no files are read.
 * The door opens (10 frames x 85 ms), its light flickers a moment, then the splash fades out over the calculator.
 * A click or a key skips it.
 */
#include "splash_data.h"

#define SPL_FRAME_MS 85
#define SPL_HOLD_MS  2200          /* door opens at 850 ms, then its light flickers until here */
#define SPL_TITLE_Y  243           /* centre of the title on the plate (frame pixels) */

typedef unsigned char u8;
static HWND spl_wnd, spl_main;
static u8 *spl_img[SPL_FRAMES];    /* BGRA, spl_px x spl_px, title drawn in */
static int spl_px, spl_cur, spl_closing, spl_alpha = 255;
static DWORD spl_start;

static void lz4_decode(const u8 *s, size_t n, u8 *d)
{
    const u8 *e = s + n;
    for (;;) {
        unsigned t = *s++;
        size_t l = t >> 4;
        if (l == 15) { u8 b; do { b = *s++; l += b; } while (b == 255); }
        memcpy(d, s, l); d += l; s += l;
        if (s >= e) break;
        size_t off = s[0] | (size_t)s[1] << 8; s += 2;
        size_t m = t & 15;
        if (m == 15) { u8 b; do { b = *s++; m += b; } while (b == 255); }
        m += 4;
        const u8 *r = d - off;
        while (m--) *d++ = *r++;
    }
}

static int spl_build(int scale)
{
    u8 *raw = VirtualAlloc(0, SPL_RAW, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!raw) return 0;
    lz4_decode(SPL_LZ4, sizeof SPL_LZ4, raw);
    const int fsz = SPL_SIZE * SPL_SIZE * 3;
    for (int i = fsz; i < SPL_RAW; i++) raw[i] = (u8)(raw[i] + raw[i - fsz]);      /* undo frame deltas */
    spl_px = SPL_SIZE * scale;
    int tx = spl_px / 2 - SPL_TITLE_W / 2, ty = SPL_TITLE_Y * scale - SPL_TITLE_H / 2;  /* title at native size */
    for (int f = 0; f < SPL_FRAMES; f++) {
        u8 *o = VirtualAlloc(0, (size_t)spl_px * spl_px * 4, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!o) return 0;
        const u8 *src = raw + (size_t)f * fsz;
        for (int y = 0; y < spl_px; y++)
            for (int x = 0; x < spl_px; x++) {
                const u8 *p = src + ((y / scale) * SPL_SIZE + x / scale) * 3;
                u8 *q = o + ((size_t)y * spl_px + x) * 4;
                q[0] = p[2]; q[1] = p[1]; q[2] = p[0]; q[3] = 255;
            }
        for (int y = 0; y < SPL_TITLE_H; y++)
            for (int x = 0; x < SPL_TITLE_W; x++) {
                const u8 *p = SPL_TITLE + (y * SPL_TITLE_W + x) * 4;
                int X = tx + x, Y = ty + y, a = p[3];
                if (!a || X < 0 || Y < 0 || X >= spl_px || Y >= spl_px) continue;
                u8 *q = o + ((size_t)Y * spl_px + X) * 4;
                q[0] = (u8)((p[2] * a + q[0] * (255 - a)) / 255);
                q[1] = (u8)((p[1] * a + q[1] * (255 - a)) / 255);
                q[2] = (u8)((p[0] * a + q[2] * (255 - a)) / 255);
            }
        spl_img[f] = o;
    }
    VirtualFree(raw, 0, MEM_RELEASE);
    return 1;
}

static void spl_show_main(void)
{
    if (spl_main && !IsWindowVisible(spl_main)) {
        ShowWindow(spl_main, SW_SHOWDEFAULT);
        UpdateWindow(spl_main);
        SetForegroundWindow(spl_main);
    }
}

static void spl_close(void) { if (!spl_closing) { spl_closing = 1; spl_show_main(); } }

static LRESULT CALLBACK SplashProc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_TIMER: {
        if (spl_closing) {                                   /* fade out over the calculator */
            spl_alpha -= 38;
            if (spl_alpha <= 0) {
                KillTimer(w, 1); DestroyWindow(w); spl_wnd = 0;
                for (int i = 0; i < SPL_FRAMES; i++) if (spl_img[i]) { VirtualFree(spl_img[i], 0, MEM_RELEASE); spl_img[i] = 0; }
                SetForegroundWindow(spl_main);
                return 0;
            }
            SetLayeredWindowAttributes(w, 0, (BYTE)spl_alpha, LWA_ALPHA);
            return 0;
        }
        DWORD t = GetTickCount() - spl_start;
        int f = (int)(t / SPL_FRAME_MS);
        if (f >= SPL_FRAMES - 1)                             /* door open: its light flickers */
            f = ((t * 5 / 2000) % 2) ? SPL_FRAMES - 2 : SPL_FRAMES - 1;
        if (f != spl_cur) { spl_cur = f; InvalidateRect(w, 0, FALSE); }
        if (t >= SPL_HOLD_MS) spl_close();
        return 0;
    }
    case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_KEYDOWN: spl_close(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(w, &ps);
        BITMAPINFO bi = {0};
        bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = spl_px; bi.bmiHeader.biHeight = -spl_px;
        bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
        if (spl_img[spl_cur])
            SetDIBitsToDevice(dc, 0, 0, spl_px, spl_px, 0, 0, 0, spl_px, spl_img[spl_cur], &bi, DIB_RGB_COLORS);
        EndPaint(w, &ps);
        return 0;
    }
    }
    return DefWindowProcW(w, msg, wp, lp);
}

/* Shows the splash; the main window (created hidden) is shown when it fades. Returns 0 if it could not start. */
static int splash_start(HINSTANCE inst, HWND main_wnd)
{
    spl_main = main_wnd;
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    if (!spl_build(sh >= 700 ? 2 : 1)) return 0;
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = SplashProc; wc.hInstance = inst; wc.lpszClassName = L"MXLCapybaraSplash";
    wc.hCursor = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    RegisterClassW(&wc);
    spl_wnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED, wc.lpszClassName, L"MXL Capybara Calculator",
                              WS_POPUP, (sw - spl_px) / 2, (sh - spl_px) / 2, spl_px, spl_px, 0, 0, inst, 0);
    if (!spl_wnd) return 0;
    SetLayeredWindowAttributes(spl_wnd, 0, 255, LWA_ALPHA);
    spl_start = GetTickCount();
    ShowWindow(spl_wnd, SW_SHOWNA);
    UpdateWindow(spl_wnd);
    SetForegroundWindow(spl_wnd);
    SetTimer(spl_wnd, 1, 40, 0);
    return 1;
}
