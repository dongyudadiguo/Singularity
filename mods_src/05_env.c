#include "../cvm_host.h"
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static Host *H;

enum {
    K_ESC = 27,
    K_ENTER = 13,
    K_BACK = 8,
    K_DEL = 127,
    K_TAB = 9,
    K_SPACE = 32,

    K_LEFT = 1000,
    K_RIGHT,
    K_UP,
    K_DOWN,
    K_HOME,
    K_END,
    K_PGUP,
    K_PGDN
};

static HWND win;
static HDC memdc;
static HBITMAP bmp;
static int W = 1000;
static int HH = 700;

static u8 q[64][12];
static int qh, qt;

static uint32_t u32(u8 *p) {
    uint32_t x = 0;
    memcpy(&x, p, 4);
    return x;
}

static void put32(u8 *p, uint32_t x) {
    memcpy(p, &x, 4);
}

static uint32_t pop32() {
    Buf b = H->pop();
    uint32_t x = b.n >= 4 ? u32(b.p) : 0;
    free(b.p);
    return x;
}

static void push32(uint32_t x) {
    u8 b[4];
    put32(b, x);
    H->push(b, 4);
}

static COLORREF col(uint32_t x) {
    return RGB((x >> 16) & 255, (x >> 8) & 255, x & 255);
}

static wchar_t *widen(u8 *p, DWORD n) {
    int m = MultiByteToWideChar(CP_UTF8, 0, (char *)p, n, 0, 0);
    wchar_t *w = calloc(m + 1, sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, (char *)p, n, w, m);
    return w;
}

static uint32_t mods() {
    uint32_t m = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000) m |= 1;
    if (GetKeyState(VK_CONTROL) & 0x8000) m |= 2;
    if (GetKeyState(VK_MENU) & 0x8000) m |= 4;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) m |= 8;

    return m;
}

static void key(uint32_t code, uint32_t ch) {
    put32(q[qt] + 0, code);
    put32(q[qt] + 4, mods());
    put32(q[qt] + 8, ch);

    qt = (qt + 1) & 63;

    if (qt == qh)
        qh = (qh + 1) & 63;
}

static int special(WPARAM w) {
    switch (w) {
    case VK_ESCAPE: return K_ESC;
    case VK_RETURN: return K_ENTER;
    case VK_BACK: return K_BACK;
    case VK_DELETE: return K_DEL;
    case VK_TAB: return K_TAB;
    case VK_LEFT: return K_LEFT;
    case VK_RIGHT: return K_RIGHT;
    case VK_UP: return K_UP;
    case VK_DOWN: return K_DOWN;
    case VK_HOME: return K_HOME;
    case VK_END: return K_END;
    case VK_PRIOR: return K_PGUP;
    case VK_NEXT: return K_PGDN;
    }
    return 0;
}

static void recreate(HWND h) {
    HDC dc = GetDC(h);

    if (memdc)
        DeleteDC(memdc);
    if (bmp)
        DeleteObject(bmp);

    memdc = CreateCompatibleDC(dc);
    bmp = CreateCompatibleBitmap(dc, W, HH);
    SelectObject(memdc, bmp);

    ReleaseDC(h, dc);
}

static LRESULT CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        BitBlt(dc, 0, 0, W, HH, memdc, 0, 0, SRCCOPY);
        EndPaint(h, &ps);
        return 0;
    }

    if (m == WM_SIZE) {
        W = LOWORD(l);
        HH = HIWORD(l);
        recreate(h);
        return 0;
    }

    if (m == WM_KEYDOWN) {
        int k = special(w);
        if (k)
            key(k, 0);
        return 0;
    }

    if (m == WM_CHAR) {
        if (w != 13 && w != 8 && w != 9 && w != 27)
            key((uint32_t)w, (uint32_t)w);
        return 0;
    }

    if (m == WM_DESTROY) {
        win = 0;
        return 0;
    }

    return DefWindowProcW(h, m, w, l);
}

static void ensure_window(wchar_t *title) {
    if (win) {
        ShowWindow(win, SW_SHOW);
        return;
    }

    WNDCLASSW wc;
    memset(&wc, 0, sizeof wc);

    wc.lpfnWndProc = proc;
    wc.hInstance = GetModuleHandleW(0);
    wc.lpszClassName = L"CVM_UI";
    wc.hCursor = LoadCursor(0, IDC_ARROW);

    RegisterClassW(&wc);

    win = CreateWindowW(
        L"CVM_UI",
        title ? title : L"CVM",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        W,
        HH,
        0,
        0,
        wc.hInstance,
        0
    );

    recreate(win);
}

static void op_ui_open(u8 *d, uint32_t n) {
    Buf title = H->pop();
    wchar_t *w = widen(title.p, title.n);

    ensure_window(w);

    free(w);
    free(title.p);

    H->adv();
}

static void op_ui_poll(u8 *d, uint32_t n) {
    MSG msg;

    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (win) {
        InvalidateRect(win, 0, 0);
        UpdateWindow(win);
    }

    H->adv();
}

static void op_ui_size(u8 *d, uint32_t n) {
    u8 r[16];

    put32(r + 0, 0);
    put32(r + 4, 0);
    put32(r + 8, W);
    put32(r + 12, HH);

    H->push(r, 16);
    H->adv();
}

static void op_ui_close(u8 *d, uint32_t n) {
    if (win)
        DestroyWindow(win);

    win = 0;
    H->adv();
}

static void op_clear(u8 *d, uint32_t n) {
    uint32_t c = pop32();

    ensure_window(L"CVM");

    HBRUSH b = CreateSolidBrush(col(c));
    RECT r = {0, 0, W, HH};

    FillRect(memdc, &r, b);
    DeleteObject(b);

    H->adv();
}

static void op_text(u8 *d, uint32_t n) {
    Buf text = H->pop();
    uint32_t y = pop32();
    uint32_t x = pop32();

    wchar_t *w = widen(text.p, text.n);

    ensure_window(L"CVM");

    SetBkMode(memdc, TRANSPARENT);
    SetTextColor(memdc, RGB(230, 230, 230));
    TextOutW(memdc, x, y, w, lstrlenW(w));

    free(w);
    free(text.p);

    H->adv();
}

static void op_in_key(u8 *d, uint32_t n) {
    if (qh == qt) {
        H->push(0, 0);
    } else {
        H->push(q[qh], 12);
        qh = (qh + 1) & 63;
    }

    H->adv();
}

static void op_empty(u8 *d, uint32_t n) {
    H->push(0, 0);
    H->adv();
}

static void op_drop(u8 *d, uint32_t n) {
    Buf a = H->pop();
    free(a.p);
    H->adv();
}

static void op_time_ms(u8 *d, uint32_t n) {
    push32(GetTickCount());
    H->adv();
}

static void op_sleep(u8 *d, uint32_t n) {
    Sleep(pop32());
    H->adv();
}

static void op_fs_read(u8 *d, uint32_t n) {
    Buf path = H->pop();

    path.p = realloc(path.p, path.n + 1);
    path.p[path.n] = 0;

    HANDLE f = CreateFileA((char *)path.p, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (f == INVALID_HANDLE_VALUE) {
        H->push(0, 0);
    } else {
        DWORD sz = GetFileSize(f, 0);
        u8 *b = malloc(sz);
        DWORD got;

        ReadFile(f, b, sz, &got, 0);
        H->push(b, got);

        free(b);
        CloseHandle(f);
    }

    free(path.p);
    H->adv();
}

static void op_fs_write(u8 *d, uint32_t n) {
    Buf data = H->pop();
    Buf path = H->pop();

    path.p = realloc(path.p, path.n + 1);
    path.p[path.n] = 0;

    HANDLE f = CreateFileA((char *)path.p, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    uint32_t ok = 0;

    if (f != INVALID_HANDLE_VALUE) {
        DWORD wrote;
        ok = WriteFile(f, data.p, data.n, &wrote, 0);
        CloseHandle(f);
    }

    push32(ok);

    free(path.p);
    free(data.p);

    H->adv();
}

static void op_fs_exists(u8 *d, uint32_t n) {
    Buf path = H->pop();

    path.p = realloc(path.p, path.n + 1);
    path.p[path.n] = 0;

    push32(GetFileAttributesA((char *)path.p) != INVALID_FILE_ATTRIBUTES);

    free(path.p);
    H->adv();
}

static void op_fs_cwd(u8 *d, uint32_t n) {
    char b[MAX_PATH];
    DWORD n2 = GetCurrentDirectoryA(MAX_PATH, b);
    H->push((u8 *)b, n2);
    H->adv();
}

__declspec(dllexport)
void cvm_init(Host *h) {
    H = h;

    h->op_name("UI:OPEN", op_ui_open);
    h->op_name("UI:POLL", op_ui_poll);
    h->op_name("UI:SIZE", op_ui_size);
    h->op_name("UI:CLOSE", op_ui_close);

    h->op_name("DRAW:CLEAR", op_clear);
    h->op_name("DRAW:TEXT", op_text);

    h->op_name("IN:KEY", op_in_key);
    h->op_name("IN:CHAR", op_empty);
    h->op_name("IN:MOUSE", op_empty);

    h->op_name("CLIP:GET", op_empty);
    h->op_name("CLIP:SET", op_drop);

    h->op_name("TIME:MS", op_time_ms);
    h->op_name("TIME:SLEEP", op_sleep);

    h->op_name("FS:READ", op_fs_read);
    h->op_name("FS:WRITE", op_fs_write);
    h->op_name("FS:EXISTS", op_fs_exists);
    h->op_name("FS:CWD", op_fs_cwd);
}