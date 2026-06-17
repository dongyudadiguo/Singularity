// 05_env — UI、绘制、输入、时间、文件系统
// gcc -shared mods_src/05_env.c -Os -s -o mods/05_env.dll -lgdi32 -luser32

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define H 32

typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;

typedef struct {
    void (*op)(u8 *, void (*)(void));
    void (*op_name)(char *, void (*)(void));
    void (*del)(u8 *);
    void (*del_name)(char *);
    void (*override)(u8 *, u8 *, uint32_t);
    void (*touch)(void);
    Buf (*rpc)(uint8_t, u8 *, uint32_t);
    void (*run)(u8 *);
    void (*enter)(u8 *);
    void (*adv)(void);
    void (*push)(u8 *, uint32_t);
    Buf (*pop)(void);
    Buf *(*top)(void);
    void *cur;
    u8 *pay; uint32_t plen;
    void (*next)(void);
    void (*next_noadv)(void);
} Host;

static Host *h;

static uint32_t U(u8 *p) { uint32_t x; memcpy(&x, p, 4); return x; }
static void WU(u8 *p, uint32_t v) { memcpy(p, &v, 4); }
static void push_u32(uint32_t v) { u8 buf[4]; WU(buf, v); h->push(buf, 4); }

// Simple window state
static HWND hwnd = NULL;
static HDC hdc = NULL;
static int win_w = 800, win_h = 600;

// UI: window operations
static void ui_open(void) {
    if (hwnd) { h->next(); return; }
    
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "CVMWnd";
    RegisterClassExA(&wc);
    
    uint32_t w = 800, height = 600;
    if (h->plen >= 8) { w = U(h->pay); height = U(h->pay + 4); }
    win_w = w; win_h = height;
    
    hwnd = CreateWindowExA(0, "CVMWnd", "CVM", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, w, height, NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    hdc = GetDC(hwnd);
    h->next();
}

static void ui_poll(void) {
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    h->next();
}

static void ui_size(void) {
    if (hwnd) {
        RECT r; GetWindowRect(hwnd, &r);
        push_u32(r.right - r.left);
        push_u32(r.bottom - r.top);
    } else {
        push_u32(0);
        push_u32(0);
    }
    h->next();
}

static void ui_close(void) {
    if (hdc) { ReleaseDC(hwnd, hdc); hdc = NULL; }
    if (hwnd) { DestroyWindow(hwnd); hwnd = NULL; }
    UnregisterClassA("CVMWnd", GetModuleHandleA(NULL));
    h->next();
}

static void draw_clear(void) {
    if (hdc) {
        RECT r = {0, 0, win_w, win_h};
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &r, brush);
        DeleteObject(brush);
    }
    h->next();
}

static void draw_text(void) {
    if (!hdc) { h->next(); return; }

    Buf text  = h->pop();
    Buf bcol  = h->pop();
    Buf by    = h->pop();
    Buf bx    = h->pop();

    uint32_t color = bcol.n >= 4 ? U(bcol.p) : 0xFFFFFF;
    int y = by.n >= 4 ? (int)U(by.p) : 0;
    int x = bx.n >= 4 ? (int)U(bx.p) : 0;

    SetBkColor(hdc, RGB(0, 0, 0));
    SetTextColor(hdc, RGB(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF));

    char *buf = malloc(text.n + 1);
    if (buf) {
        if (text.n) memcpy(buf, text.p, text.n);
        buf[text.n] = 0;
        TextOutA(hdc, x, y, buf, (int)text.n);
        free(buf);
    }
    h->next();
}

static void in_key(void) {
    u8 k[H] = {0};
    
    if (hwnd) {
        for (int vkey = 0; vkey < 256; vkey++) {
            if (GetAsyncKeyState(vkey) & 0x8000) {
                k[0] = (u8)vkey;
                break;
            }
        }
    }
    
    h->push(k, H);
    h->next();
}

static void clip_get(void) {
    if (!OpenClipboard(NULL)) { h->push(0, 0); h->next(); return; }
    
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData) {
        char *data = (char *)GlobalLock(hData);
        if (data) {
            size_t len = strlen(data);
            u8 *buf = malloc(len);
            memcpy(buf, data, len);
            h->push(buf, len);
            free(buf);
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    h->next();
}

static void clip_set(void) {
    Buf text = h->pop();
    if (!OpenClipboard(NULL)) { h->next(); return; }
    
    EmptyClipboard();
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.n + 1);
    if (hMem) {
        char *mem = (char *)GlobalLock(hMem);
        memcpy(mem, text.p, text.n);
        mem[text.n] = 0;
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
    }
    CloseClipboard();
    h->next();
}

static void time_ms(void) {
    push_u32(GetTickCount());
    h->next();
}

static void time_sleep(void) {
    if (h->plen >= 4) {
        DWORD ms = U(h->pay);
        Sleep(ms);
    }
    h->next();
}

static void fs_read(void) {
    Buf path = h->pop();
    if (path.n == 0) { h->push(0, 0); h->next(); return; }
    
    char filepath[MAX_PATH];
    memcpy(filepath, path.p, path.n < MAX_PATH ? path.n : MAX_PATH - 1);
    filepath[path.n < MAX_PATH ? path.n : MAX_PATH - 1] = 0;
    
    HANDLE hf = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hf == INVALID_HANDLE_VALUE) { h->push(0, 0); h->next(); return; }
    
    DWORD size = GetFileSize(hf, NULL);
    u8 *buf = malloc(size);
    DWORD read;
    ReadFile(hf, buf, size, &read, NULL);
    CloseHandle(hf);
    
    h->push(buf, read);
    free(buf);
    h->next();
}

static void fs_write(void) {
    Buf data = h->pop();
    Buf path = h->pop();
    
    char filepath[MAX_PATH];
    memcpy(filepath, path.p, path.n < MAX_PATH ? path.n : MAX_PATH - 1);
    filepath[path.n < MAX_PATH ? path.n : MAX_PATH - 1] = 0;
    
    HANDLE hf = CreateFileA(filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hf == INVALID_HANDLE_VALUE) { h->next(); return; }
    
    DWORD written;
    WriteFile(hf, data.p, data.n, &written, NULL);
    CloseHandle(hf);
    h->next();
}

static void fs_exists(void) {
    Buf path = h->pop();
    
    char filepath[MAX_PATH];
    memcpy(filepath, path.p, path.n < MAX_PATH ? path.n : MAX_PATH - 1);
    filepath[path.n < MAX_PATH ? path.n : MAX_PATH - 1] = 0;
    
    DWORD attrs = GetFileAttributesA(filepath);
    push_u32(attrs != INVALID_FILE_ATTRIBUTES ? 1 : 0);
    h->next();
}

static void fs_cwd(void) {
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    h->push((u8 *)cwd, strlen(cwd));
    h->next();
}

void cvm_init(Host *host) {
    h = host;
    h->op_name("UI:OPEN", ui_open);
    h->op_name("UI:POLL", ui_poll);
    h->op_name("UI:SIZE", ui_size);
    h->op_name("UI:CLOSE", ui_close);
    h->op_name("DRAW:CLEAR", draw_clear);
    h->op_name("DRAW:TEXT", draw_text);
    h->op_name("IN:KEY", in_key);
    h->op_name("CLIP:GET", clip_get);
    h->op_name("CLIP:SET", clip_set);
    h->op_name("TIME:MS", time_ms);
    h->op_name("TIME:SLEEP", time_sleep);
    h->op_name("FS:READ", fs_read);
    h->op_name("FS:WRITE", fs_write);
    h->op_name("FS:EXISTS", fs_exists);
    h->op_name("FS:CWD", fs_cwd);
}