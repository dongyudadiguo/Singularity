#include "cvm_mod.h"

static Host *G;
static HWND W;
static int mx, my, mb, wh;
static u8 q[4096];
static int qh, qt;
static u8 keys[256];

static COLORREF color(u8 *p) {
    return RGB(p[0], p[1], p[2]);
}

static LRESULT CALLBACK proc(HWND w, UINT m, WPARAM a, LPARAM l) {
    switch (m) {
    case WM_DESTROY: ExitProcess(0);
    case WM_KEYDOWN: keys[a & 255] = 1; return 0;
    case WM_KEYUP: keys[a & 255] = 0; return 0;
    case WM_CHAR: q[qt++ & 4095] = (u8)a; return 0;
    case WM_MOUSEMOVE: mx = (short)LOWORD(l); my = (short)HIWORD(l); return 0;
    case WM_LBUTTONDOWN: mb |= 1; return 0;
    case WM_LBUTTONUP: mb &= ~1; return 0;
    case WM_RBUTTONDOWN: mb |= 2; return 0;
    case WM_RBUTTONUP: mb &= ~2; return 0;
    case WM_MOUSEWHEEL: wh += GET_WHEEL_DELTA_WPARAM(a); return 0;
    }
    return DefWindowProcA(w, m, a, l);
}

static void pump() {
    MSG m;
    while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
}

static int win_open(u8 *d, uint32_t n) {
    int w = 900, h = 600;
    char title[256] = "CVM";
    WNDCLASSA c = {0};

    if (n >= 8) {
        w = rd32(d);
        h = rd32(d + 4);
    }

    if (n > 8) {
        DWORD l = n - 8;
        if (l > 255) l = 255;
        memcpy(title, d + 8, l);
        title[l] = 0;
    }

    c.lpfnWndProc = proc;
    c.hInstance = GetModuleHandleA(0);
    c.lpszClassName = "CVMWIN";
    c.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&c);

    W = CreateWindowA(
        "CVMWIN", title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        w, h,
        0, 0, c.hInstance, 0
    );

    ShowWindow(W, SW_SHOW);
    return 0;
}

static int win_poll(u8 *d, uint32_t n) {
    pump();
    return 0;
}

static int win_size(u8 *d, uint32_t n) {
    RECT r;
    u8 o[8];

    GetClientRect(W, &r);
    wr32(o, r.right - r.left);
    wr32(o + 4, r.bottom - r.top);

    G->push(o, 8);
    return 0;
}

static int win_close(u8 *d, uint32_t n) {
    DestroyWindow(W);
    return 0;
}

static int draw_clear(u8 *d, uint32_t n) {
    RECT r;
    HDC dc = GetDC(W);
    HBRUSH br;

    GetClientRect(W, &r);
    br = CreateSolidBrush(n >= 4 ? color(d) : RGB(0, 0, 0));

    FillRect(dc, &r, br);

    DeleteObject(br);
    ReleaseDC(W, dc);
    return 0;
}

static int draw_rect(u8 *d, uint32_t n) {
    HDC dc = GetDC(W);
    HBRUSH br = CreateSolidBrush(color(d + 16));
    RECT r = {
        rd32(d),
        rd32(d + 4),
        rd32(d) + rd32(d + 8),
        rd32(d + 4) + rd32(d + 12)
    };

    FillRect(dc, &r, br);

    DeleteObject(br);
    ReleaseDC(W, dc);
    return 0;
}

static int draw_line(u8 *d, uint32_t n) {
    HDC dc = GetDC(W);
    HPEN p = CreatePen(PS_SOLID, 1, color(d + 16));

    SelectObject(dc, p);
    MoveToEx(dc, rd32(d), rd32(d + 4), 0);
    LineTo(dc, rd32(d + 8), rd32(d + 12));

    DeleteObject(p);
    ReleaseDC(W, dc);
    return 0;
}

static void textout(DWORD x, DWORD y, DWORD sz, u8 *c, u8 *s, DWORD n) {
    HDC dc = GetDC(W);
    HFONT f = CreateFontA(-(int)sz, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                          0, 0, 0, 0, "Consolas");

    SelectObject(dc, f);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color(c));
    TextOutA(dc, x, y, (char *)s, n);

    DeleteObject(f);
    ReleaseDC(W, dc);
}

static int draw_text(u8 *d, uint32_t n) {
    if (n >= 16) {
        textout(rd32(d), rd32(d + 4), rd32(d + 8), d + 12, d + 16, n - 16);
    } else {
        Buf s = G->pop(), c = G->pop(), sz = G->pop(), y = G->pop(), x = G->pop();
        textout(rd32(x.p), rd32(y.p), rd32(sz.p), c.p, s.p, s.n);
        free(x.p); free(y.p); free(sz.p); free(c.p); free(s.p);
    }

    return 0;
}

static int in_key(u8 *d, uint32_t n) {
    uint32_t vk;
    u8 o[4];

    if (n >= 4) vk = rd32(d);
    else {
        Buf a = G->pop();
        vk = rd32(a.p);
        free(a.p);
    }

    wr32(o, keys[vk & 255] || (GetAsyncKeyState(vk) & 0x8000));
    G->push(o, 4);

    return 0;
}

static int in_char(u8 *d, uint32_t n) {
    u8 z = 0;

    pump();

    if (qh != qt) G->push(q + ((qh++) & 4095), 1);
    else G->push(&z, 0);

    return 0;
}

static int in_mouse(u8 *d, uint32_t n) {
    u8 o[16];

    pump();

    wr32(o, mx);
    wr32(o + 4, my);
    wr32(o + 8, mb);
    wr32(o + 12, wh);

    wh = 0;

    G->push(o, 16);
    return 0;
}

static int clip_get(u8 *d, uint32_t n) {
    HANDLE h;
    char *p;

    OpenClipboard(W);
    h = GetClipboardData(CF_TEXT);
    p = GlobalLock(h);

    G->push((u8 *)p, strlen(p));

    GlobalUnlock(h);
    CloseClipboard();

    return 0;
}

static int clip_set(u8 *d, uint32_t n) {
    Buf a;
    int own = !n;
    HGLOBAL h;
    char *p;

    if (n) a.p = d, a.n = n;
    else a = G->pop();

    OpenClipboard(W);
    EmptyClipboard();

    h = GlobalAlloc(GMEM_MOVEABLE, a.n + 1);
    p = GlobalLock(h);
    memcpy(p, a.p, a.n);
    p[a.n] = 0;
    GlobalUnlock(h);

    SetClipboardData(CF_TEXT, h);
    CloseClipboard();

    if (own) free(a.p);
    return 0;
}

static int time_ms(u8 *d, uint32_t n) {
    u8 o[4];
    wr32(o, GetTickCount());
    G->push(o, 4);
    return 0;
}

static int time_sleep(u8 *d, uint32_t n) {
    DWORD ms;

    if (n >= 4) ms = rd32(d);
    else {
        Buf a = G->pop();
        ms = rd32(a.p);
        free(a.p);
    }

    Sleep(ms);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:WIN:OPEN", win_open);
    h->op_name("CVM1:WIN:POLL", win_poll);
    h->op_name("CVM1:WIN:SIZE", win_size);
    h->op_name("CVM1:WIN:CLOSE", win_close);

    h->op_name("CVM1:DRAW:CLEAR", draw_clear);
    h->op_name("CVM1:DRAW:RECT", draw_rect);
    h->op_name("CVM1:DRAW:LINE", draw_line);
    h->op_name("CVM1:DRAW:TEXT", draw_text);

    h->op_name("CVM1:IN:KEY", in_key);
    h->op_name("CVM1:IN:CHAR", in_char);
    h->op_name("CVM1:IN:MOUSE", in_mouse);

    h->op_name("CVM1:CLIP:GET", clip_get);
    h->op_name("CVM1:CLIP:SET", clip_set);

    h->op_name("CVM1:TIME:MS", time_ms);
    h->op_name("CVM1:TIME:SLEEP", time_sleep);
}
