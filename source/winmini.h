/* Minimal Win32 declarations for a CRT-free x64 build (no Windows SDK available). */
#pragma once
typedef unsigned long long size_t;
typedef unsigned long long UINT_PTR, WPARAM;
typedef long long INT_PTR, LONG_PTR, LPARAM, LRESULT;
typedef unsigned int UINT, DWORD;
typedef int BOOL, LONG;
typedef unsigned short WORD, wchar_t;
typedef void *HANDLE, *HWND, *HINSTANCE, *HMODULE, *HMENU, *HFONT, *HBRUSH, *HICON, *HCURSOR, *HDC, *HGDIOBJ;
typedef const wchar_t *LPCWSTR;
typedef wchar_t *LPWSTR;
#define WINAPI __stdcall
#define CALLBACK __stdcall
#define DLLIMP __declspec(dllimport)
#define FALSE 0
#define TRUE 1
typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct { LONG x, y; } POINT;
typedef struct { LONG left, top, right, bottom; } RECT;
typedef struct { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; DWORD lPrivate; } MSG;
typedef struct { UINT style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor;
                 HBRUSH hbrBackground; LPCWSTR lpszMenuName, lpszClassName; } WNDCLASSW;
typedef struct { POINT ptReserved, ptMaxSize, ptMaxPosition, ptMinTrackSize, ptMaxTrackSize; } MINMAXINFO;
typedef struct { DWORD dwSize, dwICC; } INITCOMMONCONTROLSEX;
typedef struct { HWND hwndFrom; UINT_PTR idFrom; UINT code; } NMHDR;
typedef struct { UINT mask; int iItem, iSubItem; UINT state, stateMask; LPWSTR pszText; int cchTextMax, iImage; LPARAM lParam;
                 int iIndent, iGroupId; UINT cColumns; UINT *puColumns; int *piColFmt; int iGroup; } LVITEMW;
typedef struct { NMHDR hdr; LVITEMW item; } NMLVDISPINFOW;
typedef struct { NMHDR hdr; int iItem, iSubItem; UINT uNewState, uOldState, uChanged; POINT ptAction; LPARAM lParam; } NMLISTVIEW;
typedef struct { UINT mask; int fmt, cx; LPWSTR pszText; int cchTextMax, iSubItem, iImage, iOrder, cxMin, cxDefault, cxIdeal; } LVCOLUMNW;

DLLIMP HWND WINAPI CreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void *);
DLLIMP LRESULT WINAPI SendMessageW(HWND, UINT, WPARAM, LPARAM);
DLLIMP int WINAPI GetWindowTextW(HWND, LPWSTR, int);
DLLIMP BOOL WINAPI SetWindowTextW(HWND, LPCWSTR);
DLLIMP BOOL WINAPI MoveWindow(HWND, int, int, int, int, BOOL);
DLLIMP BOOL WINAPI GetClientRect(HWND, RECT *);
DLLIMP BOOL WINAPI InvalidateRect(HWND, const RECT *, BOOL);
DLLIMP LRESULT WINAPI DefWindowProcW(HWND, UINT, WPARAM, LPARAM);
DLLIMP void WINAPI PostQuitMessage(int);
DLLIMP WORD WINAPI RegisterClassW(const WNDCLASSW *);
DLLIMP HCURSOR WINAPI LoadCursorW(HINSTANCE, LPCWSTR);
DLLIMP HICON WINAPI LoadIconW(HINSTANCE, LPCWSTR);
DLLIMP BOOL WINAPI ShowWindow(HWND, int);
DLLIMP BOOL WINAPI GetMessageW(MSG *, HWND, UINT, UINT);
DLLIMP BOOL WINAPI IsDialogMessageW(HWND, MSG *);
DLLIMP BOOL WINAPI TranslateMessage(const MSG *);
DLLIMP LRESULT WINAPI DispatchMessageW(const MSG *);
DLLIMP HBRUSH WINAPI GetSysColorBrush(int);
DLLIMP HFONT WINAPI CreateFontW(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCWSTR);
DLLIMP int WINAPI SetBkMode(HDC, int);
DLLIMP HMODULE WINAPI GetModuleHandleW(LPCWSTR);
DLLIMP void WINAPI ExitProcess(UINT);
DLLIMP BOOL WINAPI InitCommonControlsEx(const INITCOMMONCONTROLSEX *);

/* splash */
typedef unsigned char BYTE;
typedef struct { HDC hdc; BOOL fErase; RECT rcPaint; BOOL fRestore, fIncUpdate; BYTE rgbReserved[32]; } PAINTSTRUCT;
typedef struct { DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount; DWORD biCompression, biSizeImage;
                 LONG biXPelsPerMeter, biYPelsPerMeter; DWORD biClrUsed, biClrImportant; } BITMAPINFOHEADER;
typedef struct { BITMAPINFOHEADER bmiHeader; DWORD bmiColors[1]; } BITMAPINFO;
DLLIMP int WINAPI GetSystemMetrics(int);
DLLIMP UINT_PTR WINAPI SetTimer(HWND, UINT_PTR, UINT, void *);
DLLIMP BOOL WINAPI KillTimer(HWND, UINT_PTR);
DLLIMP HDC WINAPI BeginPaint(HWND, PAINTSTRUCT *);
DLLIMP BOOL WINAPI EndPaint(HWND, const PAINTSTRUCT *);
DLLIMP BOOL WINAPI SetLayeredWindowAttributes(HWND, DWORD, BYTE, DWORD);
DLLIMP BOOL WINAPI DestroyWindow(HWND);
DLLIMP BOOL WINAPI UpdateWindow(HWND);
DLLIMP BOOL WINAPI SetForegroundWindow(HWND);
DLLIMP BOOL WINAPI IsWindowVisible(HWND);
DLLIMP int WINAPI SetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int, int, UINT, UINT, const void *, const BITMAPINFO *, UINT);
DLLIMP void *WINAPI VirtualAlloc(void *, size_t, DWORD, DWORD);
DLLIMP BOOL WINAPI VirtualFree(void *, size_t, DWORD);
DLLIMP DWORD WINAPI GetTickCount(void);
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define WS_POPUP 0x80000000u
#define WS_EX_TOPMOST 0x8u
#define WS_EX_TOOLWINDOW 0x80u
#define WS_EX_LAYERED 0x80000u
#define LWA_ALPHA 2
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_RELEASE 0x8000
#define PAGE_READWRITE 4
#define DIB_RGB_COLORS 0
#define BI_RGB 0
#define SW_SHOWNA 8
#define WM_PAINT 0x000F
#define WM_ERASEBKGND 0x0014
#define WM_KEYDOWN 0x0100
#define WM_TIMER 0x0113
#define WM_LBUTTONDOWN 0x0201
#define WM_RBUTTONDOWN 0x0204

#define MAKEINTRESOURCEW(i) ((LPWSTR)(UINT_PTR)(WORD)(i))
#define LOWORD(l) ((WORD)((UINT_PTR)(l) & 0xffff))
#define HIWORD(l) ((WORD)(((UINT_PTR)(l) >> 16) & 0xffff))
#define WS_CHILD 0x40000000u
#define WS_VISIBLE 0x10000000u
#define WS_TABSTOP 0x00010000u
#define WS_VSCROLL 0x00200000u
#define WS_OVERLAPPEDWINDOW 0x00CF0000u
#define WS_EX_CLIENTEDGE 0x200u
#define ES_AUTOHSCROLL 0x80u
#define ES_NUMBER 0x2000u
#define CBS_DROPDOWNLIST 3u
#define BS_AUTOCHECKBOX 3u
#define SS_LEFTNOWORDWRAP 0xCu
#define CW_USEDEFAULT ((int)0x80000000)
#define SW_SHOWDEFAULT 10
#define IDC_ARROW MAKEINTRESOURCEW(32512)
#define COLOR_BTNFACE 15
#define FW_NORMAL 400
#define DEFAULT_CHARSET 1
#define CLEARTYPE_QUALITY 5
#define TRANSPARENT 1
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_SIZE 0x0005
#define WM_GETMINMAXINFO 0x0024
#define WM_SETFONT 0x0030
#define WM_NOTIFY 0x004E
#define WM_COMMAND 0x0111
#define WM_CTLCOLORSTATIC 0x0138
#define EN_CHANGE 0x0300
#define BN_CLICKED 0
#define BM_GETCHECK 0x00F0
#define BST_CHECKED 1
#define CB_ADDSTRING 0x0143
#define CB_GETCURSEL 0x0147
#define CB_SETCURSEL 0x014E
#define CBN_SELCHANGE 1
#define ICC_LISTVIEW_CLASSES 0x1
#define ICC_STANDARD_CLASSES 0x4000
#define WC_LISTVIEWW L"SysListView32"
#define LVS_REPORT 0x1
#define LVS_SHOWSELALWAYS 0x8
#define LVS_OWNERDATA 0x1000
#define LVS_EX_GRIDLINES 0x1
#define LVS_EX_FULLROWSELECT 0x20
#define LVS_EX_DOUBLEBUFFER 0x10000
#define LVIF_TEXT 0x1
#define LVCF_FMT 0x1
#define LVCF_WIDTH 0x2
#define LVCF_TEXT 0x4
#define LVCFMT_LEFT 0
#define LVCFMT_RIGHT 1
#define LVSICF_NOSCROLL 0x2
#define LVM_SETITEMCOUNT 0x102F
#define LVM_SETEXTENDEDLISTVIEWSTYLE 0x1036
#define LVM_INSERTCOLUMNW 0x1061
#define LVN_GETDISPINFOW (-177)
#define LVN_COLUMNCLICK (-108)
#define ListView_SetItemCountEx(h, n, f) SendMessageW((h), LVM_SETITEMCOUNT, (WPARAM)(n), (LPARAM)(f))
#define ListView_SetExtendedListViewStyle(h, s) SendMessageW((h), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)(s))
#define ListView_InsertColumn(h, i, c) SendMessageW((h), LVM_INSERTCOLUMNW, (WPARAM)(i), (LPARAM)(c))
