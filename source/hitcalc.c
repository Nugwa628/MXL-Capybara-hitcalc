/*
 * MXL Capybara Calculator - chance to hit every Median XL monster.
 * Starts with the MXL Capybara Editor's loading screen (splash.h, all data inside the exe).
 *
 * Formula (D2Game 1.13c, 0x6FCFDE90; Median XL only clamps both levels to >= 1):
 *   def   = monlvl.AC[difficulty][monster level] * monstats.AC% / 100
 *   hit   = 100 * AR / (AR + def)
 *   hit   = hit * 2 * clvl / (clvl + mlvl)
 *   clamp 5 .. 95
 * Data generated from the installed medianxl-YmludGJsdHh0.mpq (monstats.bin, monlvl.bin).
 * Freestanding Win32 (x64): clang -target x86_64-pc-windows-msvc + lld-link, no CRT.
 */
#include "winmini.h"
#include "mondata.h"

typedef long long i64;

/* ---- tiny CRT replacements ---- */
void *memset(void *d, int c, size_t n) { unsigned char *p = d; while (n--) *p++ = (unsigned char)c; return d; }
void *memcpy(void *d, const void *s, size_t n) { unsigned char *a = d; const unsigned char *b = s; while (n--) *a++ = *b++; return d; }
int _fltused = 0;

#include "splash.h"

static int wlen(const wchar_t *s) { int n = 0; while (s[n]) n++; return n; }
static void wcpy(wchar_t *d, const wchar_t *s) { while ((*d++ = *s++)); }
static void wcat(wchar_t *d, const wchar_t *s) { d += wlen(d); wcpy(d, s); }
static void itow(i64 v, wchar_t *out, int commas)
{
    wchar_t t[32]; int i = 0, g = 0; int neg = v < 0; unsigned long long u = neg ? (unsigned long long)-v : (unsigned long long)v;
    do { if (commas && g == 3) { t[i++] = L','; g = 0; } t[i++] = (wchar_t)(L'0' + u % 10); u /= 10; g++; } while (u);
    if (neg) t[i++] = L'-';
    int j = 0; while (i) out[j++] = t[--i]; out[j] = 0;
}
static i64 wtoi(const wchar_t *s)
{
    i64 v = 0; int any = 0;
    for (; *s; s++) { if (*s >= L'0' && *s <= L'9') { v = v * 10 + (*s - L'0'); any = 1; if (v > 4000000000000LL) break; } else if (*s == L',' || *s == L' ' || *s == L'.') continue; else break; }
    return any ? v : -1;
}
static wchar_t lower(wchar_t c) { return (c >= L'A' && c <= L'Z') ? c + 32 : c; }
static int contains_ci(const char *hay, const wchar_t *needle)
{
    int n = wlen(needle); if (!n) return 1;
    for (const char *h = hay; *h; h++) {
        int k = 0;
        while (k < n && h[k] && lower((wchar_t)(unsigned char)h[k]) == lower(needle[k])) k++;
        if (k == n) return 1;
    }
    return 0;
}

/* ---- game math ---- */
static int mon_level(const Mon *m, int diff, int override_lvl, int apply_override)
{
    int lv = m->lvl[diff];
    if (apply_override && override_lvl > 0 && !m->boss) lv = override_lvl;
    if (lv < 1) lv = 1;
    if (lv >= NLVL) lv = NLVL - 1;
    return lv;
}
static i64 mon_def(const Mon *m, int diff, int lv) { return (i64)MONLVL_AC[diff][lv] * m->acp[diff] / 100; }
static int hit_chance(i64 ar, i64 def, int clvl, int mlvl)
{
    if (ar < 0) ar = 0;
    if (def < 0) def = 0;
    if (clvl < 1) clvl = 1;
    if (mlvl < 1) mlvl = 1;
    i64 h = (ar + def) ? ar * 100 / (ar + def) : 100;
    h = h * 2 * clvl / (clvl + mlvl);
    if (h < 6) h = 5; else if (h > 94) h = 95;
    return (int)h;
}
/* smallest AR giving 95%, or -1 if impossible at this level difference */
static i64 ar_for_95(i64 def, int clvl, int mlvl)
{
    if (hit_chance(4000000000000LL, def, clvl, mlvl) < 95) return -1;
    i64 lo = 0, hi = 4000000000000LL;
    while (lo < hi) { i64 mid = lo + (hi - lo) / 2; if (hit_chance(mid, def, clvl, mlvl) >= 95) hi = mid; else lo = mid + 1; }
    return lo;
}

/* ---- UI ---- */
enum { ID_AR = 101, ID_CLVL, ID_DIFF, ID_MLVL, ID_SEARCH, ID_BOSSONLY, ID_LIST, ID_INFO };
static HWND hAR, hCL, hDiff, hML, hSearch, hBoss, hList, hInfo;
static HFONT hFont;
static int g_rows[NMONS + 1], g_nrows;
static int g_hit[NMONS + 1]; static i64 g_def[NMONS + 1], g_need[NMONS + 1]; static int g_lv[NMONS + 1];
static int g_sortcol = 0, g_sortdesc = 0;
static wchar_t g_cell[128];

static i64 get_num(HWND h) { wchar_t b[64]; GetWindowTextW(h, b, 64); return wtoi(b); }

static int cmp_rows(int a, int b)
{
    i64 x = 0, y = 0;
    switch (g_sortcol) {
    case 0: { const char *p = MONS[a].name, *q = MONS[b].name;
              while (*p && lower(*p) == lower(*q)) { p++; q++; }
              x = lower(*p); y = lower(*q); if (x == y) { x = a; y = b; } break; }
    case 1: x = MONS[a].boss; y = MONS[b].boss; break;
    case 2: x = g_lv[a]; y = g_lv[b]; break;
    case 3: x = g_def[a]; y = g_def[b]; break;
    case 4: x = g_hit[a]; y = g_hit[b]; break;
    case 5: x = g_need[a] < 0 ? 0x7fffffffffffffffLL : g_need[a]; y = g_need[b] < 0 ? 0x7fffffffffffffffLL : g_need[b]; break;
    }
    if (x == y && g_sortcol) { /* tie-break by name */
        const char *p = MONS[a].name, *q = MONS[b].name; while (*p && lower(*p) == lower(*q)) { p++; q++; } x = lower(*p); y = lower(*q);
        if (x == y) { x = a; y = b; }
        return x < y ? -1 : x > y;
    }
    int r = x < y ? -1 : x > y;
    return g_sortdesc ? -r : r;
}
static void sort_rows(void)
{   /* shell sort, n ~ 2.5k */
    for (int gap = g_nrows / 2; gap > 0; gap /= 2)
        for (int i = gap; i < g_nrows; i++) {
            int t = g_rows[i], j = i;
            while (j >= gap && cmp_rows(g_rows[j - gap], t) > 0) { g_rows[j] = g_rows[j - gap]; j -= gap; }
            g_rows[j] = t;
        }
}

static void recalc(void)
{
    if (!hList || !hAR || !hCL || !hDiff || !hML || !hSearch || !hBoss || !hInfo) return;
    i64 ar = get_num(hAR); if (ar < 0) ar = 0;
    i64 cl = get_num(hCL); if (cl < 1) cl = 1; if (cl > 999) cl = 999;
    int diff = (int)SendMessageW(hDiff, CB_GETCURSEL, 0, 0); if (diff < 0) diff = 2;
    i64 ml = get_num(hML);
    int bossonly = SendMessageW(hBoss, BM_GETCHECK, 0, 0) == BST_CHECKED;
    wchar_t q[128]; GetWindowTextW(hSearch, q, 128);

    g_nrows = 0;
    for (int i = 0; i < (int)NMONS; i++) {
        const Mon *m = &MONS[i];
        if (m->lvl[diff] == 0 && m->acp[diff] == 0) continue;
        if (bossonly && !m->boss) continue;
        if (!contains_ci(m->name, q)) continue;
        int lv = mon_level(m, diff, (int)ml, ml > 0);
        g_lv[i] = lv;
        g_def[i] = mon_def(m, diff, lv);
        g_hit[i] = hit_chance(ar, g_def[i], (int)cl, lv);
        g_need[i] = ar_for_95(g_def[i], (int)cl, lv);
        g_rows[g_nrows++] = i;
    }
    sort_rows();
    ListView_SetItemCountEx(hList, g_nrows, LVSICF_NOSCROLL);
    InvalidateRect(hList, 0, FALSE);

    wchar_t info[256], n[32];
    wcpy(info, L"Showing "); itow(g_nrows, n, 1); wcat(info, n); wcat(info, L" monsters.  Bosses keep their own level; other monsters use the Monster level box (or their table level if it's empty).");
    SetWindowTextW(hInfo, info);
}

static HWND mk(const wchar_t *cls, const wchar_t *txt, DWORD style, int x, int y, int w, int h, HWND parent, int id, DWORD ex)
{
    HWND c = CreateWindowExW(ex, cls, txt, WS_CHILD | WS_VISIBLE | style, x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(0), 0);
    SendMessageW(c, WM_SETFONT, (WPARAM)hFont, TRUE);
    return c;
}

static void layout(HWND w)
{
    if (!hList) return;
    RECT r; GetClientRect(w, &r);
    MoveWindow(hList, 10, 96, r.right - 20, r.bottom - 106, TRUE);
    MoveWindow(hInfo, 10, 72, r.right - 20, 18, TRUE);
}

static void add_col(int i, const wchar_t *t, int w, int right)
{
    LVCOLUMNW c = {0};
    c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT; c.pszText = (wchar_t *)t; c.cx = w; c.fmt = right ? LVCFMT_RIGHT : LVCFMT_LEFT;
    ListView_InsertColumn(hList, i, &c);
}

static LRESULT CALLBACK WndProc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        hFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        mk(L"STATIC", L"Attack Rating", 0, 10, 12, 100, 20, w, 0, 0);
        hAR = mk(L"EDIT", L"100000", ES_AUTOHSCROLL | WS_TABSTOP, 10, 34, 130, 24, w, ID_AR, WS_EX_CLIENTEDGE);
        mk(L"STATIC", L"Character Level", 0, 155, 12, 110, 20, w, 0, 0);
        hCL = mk(L"EDIT", L"140", ES_NUMBER | WS_TABSTOP, 155, 34, 90, 24, w, ID_CLVL, WS_EX_CLIENTEDGE);
        mk(L"STATIC", L"Difficulty", 0, 260, 12, 90, 20, w, 0, 0);
        hDiff = mk(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 260, 33, 110, 200, w, ID_DIFF, 0);
        SendMessageW(hDiff, CB_ADDSTRING, 0, (LPARAM)L"Normal");
        SendMessageW(hDiff, CB_ADDSTRING, 0, (LPARAM)L"Nightmare");
        SendMessageW(hDiff, CB_ADDSTRING, 0, (LPARAM)L"Hell");
        SendMessageW(hDiff, CB_SETCURSEL, 2, 0);
        mk(L"STATIC", L"Monster level (area)", 0, 385, 12, 140, 20, w, 0, 0);
        hML = mk(L"EDIT", L"", ES_NUMBER | WS_TABSTOP, 385, 34, 120, 24, w, ID_MLVL, WS_EX_CLIENTEDGE);
        mk(L"STATIC", L"Search", 0, 520, 12, 80, 20, w, 0, 0);
        hSearch = mk(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, 520, 34, 200, 24, w, ID_SEARCH, WS_EX_CLIENTEDGE);
        hBoss = mk(L"BUTTON", L"Bosses only", BS_AUTOCHECKBOX | WS_TABSTOP, 735, 36, 110, 20, w, ID_BOSSONLY, 0);
        hInfo = mk(L"STATIC", L"", SS_LEFTNOWORDWRAP, 10, 72, 800, 18, w, ID_INFO, 0);
        hList = mk(WC_LISTVIEWW, L"", LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | WS_TABSTOP, 10, 96, 800, 500, w, ID_LIST, WS_EX_CLIENTEDGE);
        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        add_col(0, L"Monster", 300, 0);
        add_col(1, L"Boss", 50, 0);
        add_col(2, L"Level", 60, 1);
        add_col(3, L"Defense", 110, 1);
        add_col(4, L"Hit Chance", 90, 1);
        add_col(5, L"AR for 95%", 140, 1);
        recalc();
        return 0;
    }
    case WM_SIZE: layout(w); return 0;
    case WM_GETMINMAXINFO: { MINMAXINFO *mm = (MINMAXINFO *)lp; mm->ptMinTrackSize.x = 870; mm->ptMinTrackSize.y = 300; return 0; }
    case WM_COMMAND:
        if ((HIWORD(wp) == EN_CHANGE && (LOWORD(wp) == ID_AR || LOWORD(wp) == ID_CLVL || LOWORD(wp) == ID_MLVL || LOWORD(wp) == ID_SEARCH)) ||
            (HIWORD(wp) == CBN_SELCHANGE && LOWORD(wp) == ID_DIFF) || (LOWORD(wp) == ID_BOSSONLY && HIWORD(wp) == BN_CLICKED))
            recalc();
        return 0;
    case WM_NOTIFY: {
        NMHDR *h = (NMHDR *)lp;
        if (h->idFrom != ID_LIST) break;
        if (h->code == (UINT)LVN_GETDISPINFOW) {
            NMLVDISPINFOW *d = (NMLVDISPINFOW *)lp;
            if (!(d->item.mask & LVIF_TEXT) || d->item.iItem >= g_nrows) return 0;
            int i = g_rows[d->item.iItem];
            g_cell[0] = 0;
            switch (d->item.iSubItem) {
            case 0: { int k = 0; for (const char *p = MONS[i].name; *p && k < 120; p++) g_cell[k++] = (wchar_t)(unsigned char)*p; g_cell[k] = 0; break; }
            case 1: wcpy(g_cell, MONS[i].boss ? L"yes" : L""); break;
            case 2: itow(g_lv[i], g_cell, 0); break;
            case 3: itow(g_def[i], g_cell, 1); break;
            case 4: itow(g_hit[i], g_cell, 0); wcat(g_cell, L"%"); break;
            case 5: if (g_need[i] < 0) wcpy(g_cell, L"impossible"); else itow(g_need[i], g_cell, 1); break;
            }
            d->item.pszText = g_cell;
            return 0;
        }
        if (h->code == (UINT)LVN_COLUMNCLICK) {
            NMLISTVIEW *n = (NMLISTVIEW *)lp;
            if (n->iSubItem == g_sortcol) g_sortdesc = !g_sortdesc; else { g_sortcol = n->iSubItem; g_sortdesc = (g_sortcol >= 1 && g_sortcol <= 3); }
            sort_rows(); InvalidateRect(hList, 0, FALSE);
            return 0;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: SetBkMode((HDC)wp, TRANSPARENT); return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(w, msg, wp, lp);
}

void WinMainCRTStartup(void)
{
    INITCOMMONCONTROLSEX ic = { sizeof ic, ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&ic);
    HINSTANCE inst = GetModuleHandleW(0);
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc; wc.hInstance = inst; wc.lpszClassName = L"MXLHitCalc";
    wc.hCursor = LoadCursorW(0, (LPCWSTR)IDC_ARROW); wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    RegisterClassW(&wc);
    HWND w = CreateWindowExW(0, wc.lpszClassName, L"MXL Capybara Calculator", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 900, 680, 0, 0, inst, 0);
    if (!splash_start(inst, w)) ShowWindow(w, SW_SHOWDEFAULT);    /* shown when the splash fades */
    MSG m;
    while (GetMessageW(&m, 0, 0, 0) > 0) {
        if (!IsDialogMessageW(w, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
    }
    ExitProcess(0);
}
