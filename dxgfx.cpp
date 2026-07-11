#define WIN32_LEAN_AND_MEAN
#define DXGFX_BUILD
#include "dxgfx.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static HWND g_hwnd = 0;
static ID2D1Factory *g_d2d = 0;
static IDWriteFactory *g_dw = 0;
static ID2D1HwndRenderTarget *g_rt = 0;
static UINT32 g_rt_w = 0, g_rt_h = 0;
static ID2D1SolidColorBrush *g_brush = 0;
/* Text formats are expensive to create; cache by rounded size. */
#define DX_FMT_CACHE 16
static IDWriteTextFormat *g_fmt[DX_FMT_CACHE];
static float g_fmt_size[DX_FMT_CACHE];
static int g_fmt_n = 0;
static int g_inited = 0;
static int g_drawing = 0;
static int g_close = 0;
static int g_wheel_accum = 0; /* raw WHEEL_DELTA, messages since last snapshot */
static int g_wheel_frame = 0; /* raw WHEEL_DELTA snapshot for this frame */
static int g_wheel_latched = 0; /* 1 after first snapshot in current frame */
static char g_text[64];
static int g_text_len = 0;
static dx_u8 g_prev_keys[256];
static dx_u8 g_prev_mouse_buttons;
static float g_cam_x = 0.0f, g_cam_y = 0.0f, g_zoom = 1.0f;
static const int G_W = 1280;
static const int G_H = 720;

static LRESULT CALLBACK dxgfx_wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE) { g_close = 1; DestroyWindow(h); return 0; }
    if (m == WM_DESTROY) { g_close = 1; return 0; }
    /* Accumulate raw WHEEL_DELTA units. High-res devices send |delta|<120; integer /120 would drop those as empty scrolls. */
    if (m == WM_MOUSEWHEEL) { g_wheel_accum += (int)GET_WHEEL_DELTA_WPARAM(w); return 0; }
    if (m == WM_CHAR) {
        if (w >= 32 && w != 127 && g_text_len < (int)sizeof(g_text) - 5) {
            if (w < 0x80) g_text[g_text_len++] = (char)w;
        }
        return 0;
    }
    return DefWindowProcA(h, m, w, l);
}

static void dxgfx_pump(void) {
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static D2D1_COLOR_F dxgfx_color(dx_u32 c) {
    D2D1_COLOR_F r;
    r.a = ((c >> 24) & 255) / 255.0f;
    r.r = ((c >> 16) & 255) / 255.0f;
    r.g = ((c >> 8) & 255) / 255.0f;
    r.b = (c & 255) / 255.0f;
    return r;
}

static int dxgfx_init(void) {
    if (g_inited) { dxgfx_pump(); return g_rt != 0; }
    g_inited = 1;
    /* Match mouse client pixels to D2D backing store; avoids non-1:1 pan/drag on scaled displays. */
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        typedef BOOL (WINAPI *SetDpiAwarenessContext_t)(void*);
        SetDpiAwarenessContext_t set_ctx = (SetDpiAwarenessContext_t)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (set_ctx) set_ctx((void*)(-4)); /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */
        else {
            typedef BOOL (WINAPI *SetProcessDPIAware_t)(void);
            SetProcessDPIAware_t set_aware = (SetProcessDPIAware_t)GetProcAddress(user32, "SetProcessDPIAware");
            if (set_aware) set_aware();
        }
    }

    HINSTANCE inst = GetModuleHandleA(0);
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = dxgfx_wndproc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.lpszClassName = "SingularityDirectXDrawWindow";
    RegisterClassExA(&wc);

    RECT rc = {0, 0, G_W, G_H};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowExA(0, wc.lpszClassName, "Singularity SelfEdit", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                             0, 0, inst, 0);
    if (!g_hwnd) return 0;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2d))) return 0;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_dw))) return 0;

    RECT cr;
    GetClientRect(g_hwnd, &cr);
    D2D1_SIZE_U sz = D2D1::SizeU((UINT32)(cr.right - cr.left), (UINT32)(cr.bottom - cr.top));
    D2D1_HWND_RENDER_TARGET_PROPERTIES hp = D2D1::HwndRenderTargetProperties(g_hwnd, sz, D2D1_PRESENT_OPTIONS_IMMEDIATELY);
    D2D1_RENDER_TARGET_PROPERTIES rp = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_HARDWARE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        0.0f, 0.0f, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);
    if (FAILED(g_d2d->CreateHwndRenderTarget(rp, hp, &g_rt))) return 0;
    g_rt_w = sz.width;
    g_rt_h = sz.height;
    if (FAILED(g_rt->CreateSolidColorBrush(dxgfx_color(0xffffffff), &g_brush))) return 0;
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    dxgfx_pump();
    return 1;
}

static void dxgfx_resize_target(void) {
    if (!g_hwnd || !g_rt) return;
    RECT cr;
    GetClientRect(g_hwnd, &cr);
    UINT32 w = (UINT32)(cr.right - cr.left);
    UINT32 h = (UINT32)(cr.bottom - cr.top);
    if (w == 0 || h == 0) return;
    if (w == g_rt_w && h == g_rt_h) return;
    D2D1_SIZE_U sz = D2D1::SizeU(w, h);
    if (SUCCEEDED(g_rt->Resize(sz))) {
        g_rt_w = w;
        g_rt_h = h;
    }
}

extern "C" DXGFX_API int dxgfx_frame_begin(void) {
    if (!dxgfx_init()) return 0;
    /* One pump at frame start: latest mouse/keyboard messages before simulation. */
    dxgfx_pump();
    /* Snapshot wheel ONCE per frame. frame_clear/auto_begin also call frame_begin
     * while already drawing; re-snapshot would replace a real delta with 0 and
     * kill zoom for the rest of the frame. */
    if (!g_wheel_latched) {
        g_wheel_frame = g_wheel_accum;
        g_wheel_accum = 0;
        g_wheel_latched = 1;
    }
    if (g_close) {
        /* Window closed: stop VM busy-loop so the console host exits too. */
        ExitProcess(0);
    }
    if (!IsWindowVisible(g_hwnd) && !g_close) ShowWindow(g_hwnd, SW_SHOW);
    dxgfx_resize_target();
    if (!g_drawing) { g_rt->BeginDraw(); g_drawing = 1; }
    return 1;
}

extern "C" DXGFX_API int dxgfx_clear(dx_u32 argb) {
    if (!dxgfx_frame_begin()) return 0;
    g_rt->Clear(dxgfx_color(argb));
    return 1;
}

extern "C" DXGFX_API int dxgfx_frame_end(void) {
    if (!g_rt || !g_drawing) return 0;
    /* Present (IMMEDIATELY): do not wait for vsync; lower input-to-photon latency. */
    HRESULT hr = g_rt->EndDraw();
    g_drawing = 0;
    g_wheel_latched = 0; /* allow next frame_begin to snapshot again */
    /* Drain OS queue into g_wheel_accum for the next frame snapshot. */
    dxgfx_pump();
    return SUCCEEDED(hr);
}

static int g_auto_submit = 0;

static int dxgfx_auto_begin(dx_u32 argb) {
    g_auto_submit = !g_drawing;
    if (!dxgfx_frame_begin()) { g_auto_submit = 0; return 0; }
    g_brush->SetColor(dxgfx_color(argb));
    return 1;
}

static int dxgfx_auto_end(void) {
    int submit = g_auto_submit;
    g_auto_submit = 0;
    return submit ? dxgfx_frame_end() : 1;
}

extern "C" DXGFX_API int dxgfx_screen_size(int out_size[2]) {
    if (!out_size || !dxgfx_init()) return 0;
    RECT cr;
    GetClientRect(g_hwnd, &cr);
    out_size[0] = cr.right - cr.left;
    out_size[1] = cr.bottom - cr.top;
    return 1;
}

extern "C" DXGFX_API int dxgfx_window_should_close(void) {
    dxgfx_pump();
    return g_close;
}

static dx_u8 mouse_bits(void) {
    return (dx_u8)(((GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0) |
                   ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 2 : 0) |
                   ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 4 : 0) |
                   ((GetAsyncKeyState(VK_XBUTTON1) & 0x8000) ? 8 : 0) |
                   ((GetAsyncKeyState(VK_XBUTTON2) & 0x8000) ? 16 : 0));
}

extern "C" DXGFX_API int dxgfx_keyboard(dx_u8 out_state[256]) {
    if (!out_state || !dxgfx_init()) return 0;
    for (int i = 0; i < 256; i++) out_state[i] = (GetAsyncKeyState(i) & 0x8000) ? 0x80 : 0;
    return 1;
}

static void dxgfx_rt_size(float *rw, float *rh) {
    int csz[2] = {0, 0};
    RECT cr;
    if (g_hwnd && GetClientRect(g_hwnd, &cr)) {
        csz[0] = cr.right - cr.left;
        csz[1] = cr.bottom - cr.top;
    }
    if (rw) *rw = (float)(csz[0] > 0 ? csz[0] : 1280);
    if (rh) *rh = (float)(csz[1] > 0 ? csz[1] : 720);
}

static void dxgfx_mouse_in_rt(float *mx, float *my, float *cx, float *cy) {
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(g_hwnd, &p);
    float rw, rh;
    dxgfx_rt_size(&rw, &rh);
    if (mx) *mx = (float)p.x;
    if (my) *my = (float)p.y;
    if (cx) *cx = rw * 0.5f;
    if (cy) *cy = rh * 0.5f;
}

extern "C" DXGFX_API int dxgfx_mouse(int out_state[4]) {
    if (!out_state || !dxgfx_init()) return 0;
    float mx, my, cx, cy;
    dxgfx_mouse_in_rt(&mx, &my, &cx, &cy);
    (void)cx; (void)cy;
    out_state[0] = (int)(mx + (mx >= 0 ? 0.5f : -0.5f));
    out_state[1] = (int)(my + (my >= 0 ? 0.5f : -0.5f));
    out_state[2] = mouse_bits();
    out_state[3] = g_wheel_frame;
    return 1;
}

extern "C" DXGFX_API int dxgfx_input_snapshot(dx_u8 keys_down[256], dx_u8 keys_pressed[256], dx_u8 keys_released[256], int mouse[8], char text[64]) {
    if (!dxgfx_init()) return 0;
    dxgfx_pump();
    dx_u8 cur[256];
    for (int i = 0; i < 256; i++) {
        cur[i] = (GetAsyncKeyState(i) & 0x8000) ? 1 : 0;
        if (keys_down) keys_down[i] = cur[i];
        if (keys_pressed) keys_pressed[i] = (dx_u8)(cur[i] && !g_prev_keys[i]);
        if (keys_released) keys_released[i] = (dx_u8)(!cur[i] && g_prev_keys[i]);
        g_prev_keys[i] = cur[i];
    }
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(g_hwnd, &p);
    dx_u8 mb = mouse_bits();
    if (mouse) {
        mouse[0] = p.x;
        mouse[1] = p.y;
        mouse[2] = mb;
        mouse[3] = (mb & ~g_prev_mouse_buttons);
        mouse[4] = (~mb & g_prev_mouse_buttons) & 31;
        mouse[5] = g_wheel_frame;
        mouse[6] = 0;
        mouse[7] = 0;
    }
    g_prev_mouse_buttons = mb;
    /* g_wheel_frame is raw WHEEL_DELTA snapshot for this frame; mid-frame msgs go to accum. */
    if (text) {
        int n = g_text_len;
        if (n > 63) n = 63;
        memcpy(text, g_text, n);
        text[n] = 0;
    }
    g_text_len = 0;
    g_text[0] = 0;
    return 1;
}

extern "C" DXGFX_API int dxgfx_set_camera(float target_x, float target_y, float zoom) {
    g_cam_x = target_x;
    g_cam_y = target_y;
    g_zoom = zoom > 0.05f ? zoom : 0.05f;
    return 1;
}



extern "C" DXGFX_API int dxgfx_world_mouse(float out_xy[2]) {
    if (!out_xy || !dxgfx_init()) return 0;
    float mx, my, cx, cy;
    dxgfx_mouse_in_rt(&mx, &my, &cx, &cy);
    out_xy[0] = (mx - cx) / g_zoom + g_cam_x;
    out_xy[1] = (my - cy) / g_zoom + g_cam_y;
    return 1;
}

static void world_to_screen(float x, float y, float *sx, float *sy) {
    float rw, rh;
    dxgfx_rt_size(&rw, &rh);
    *sx = (x - g_cam_x) * g_zoom + rw * 0.5f;
    *sy = (y - g_cam_y) * g_zoom + rh * 0.5f;
}

static IDWriteTextFormat *dxgfx_text_format(float size) {
    if (size <= 0.0f) size = 20.0f;
    /* quantize to 0.25px so slight float noise reuses slots */
    float key = (float)((int)(size * 4.0f + 0.5f)) * 0.25f;
    if (key < 1.0f) key = 1.0f;
    for (int i = 0; i < g_fmt_n; i++) {
        if (g_fmt[i] && g_fmt_size[i] == key) return g_fmt[i];
    }
    IDWriteTextFormat *fmt = 0;
    HRESULT hr = g_dw->CreateTextFormat(L"Consolas", 0, DWRITE_FONT_WEIGHT_NORMAL,
                                        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                        key, L"", &fmt);
    if (FAILED(hr) || !fmt) return 0;
    fmt->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    if (g_fmt_n < DX_FMT_CACHE) {
        g_fmt[g_fmt_n] = fmt;
        g_fmt_size[g_fmt_n] = key;
        g_fmt_n++;
        return fmt;
    }
    /* cache full: evict slot 0 */
    if (g_fmt[0]) g_fmt[0]->Release();
    for (int i = 1; i < DX_FMT_CACHE; i++) {
        g_fmt[i - 1] = g_fmt[i];
        g_fmt_size[i - 1] = g_fmt_size[i];
    }
    g_fmt[DX_FMT_CACHE - 1] = fmt;
    g_fmt_size[DX_FMT_CACHE - 1] = key;
    return fmt;
}

extern "C" DXGFX_API int dxgfx_measure_text(float size, const char *utf8, dx_u32 len, float out_size[2]) {
    if (!utf8 || !out_size || !dxgfx_init()) return 0;
    if (size <= 0.0f) size = 20.0f;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, 0, 0);
    if (wlen <= 0) { out_size[0] = out_size[1] = 0.0f; return 1; }
    wchar_t *ws = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    if (!ws) return 0;
    MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, ws, wlen);
    ws[wlen] = 0;

    IDWriteTextFormat *fmt = dxgfx_text_format(size);
    IDWriteTextLayout *layout = 0;
    HRESULT hr = fmt ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
        hr = g_dw->CreateTextLayout(ws, (UINT32)wlen, fmt, 100000.0f, 100000.0f, &layout);
    if (SUCCEEDED(hr)) {
        DWRITE_TEXT_METRICS metrics;
        hr = layout->GetMetrics(&metrics);
        if (SUCCEEDED(hr)) {
            out_size[0] = metrics.widthIncludingTrailingWhitespace;
            out_size[1] = metrics.height;
        }
    }
    if (layout) layout->Release();
    /* fmt cached in dxgfx_text_format */
    free(ws);
    return SUCCEEDED(hr);
}

extern "C" DXGFX_API int dxgfx_draw_text(int x, int y, dx_u32 argb, float size, const char *utf8, dx_u32 len) {
    if (!utf8) return 0;
    if (size <= 0.0f) size = 20.0f;
    if (!dxgfx_auto_begin(argb)) return 0;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, 0, 0);
    if (wlen <= 0) return dxgfx_auto_end();
    wchar_t *ws = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    if (!ws) return dxgfx_auto_end();
    MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, ws, wlen);
    ws[wlen] = 0;

    IDWriteTextFormat *fmt = dxgfx_text_format(size * g_zoom);
    HRESULT hr = fmt ? S_OK : E_FAIL;
    if (SUCCEEDED(hr)) {
        D2D1_SIZE_F s = g_rt->GetSize();
        float sx, sy;
        world_to_screen((float)x, (float)y, &sx, &sy);
        D2D1_RECT_F r = D2D1::RectF(sx, sy, s.width, s.height);
        g_rt->DrawText(ws, (UINT32)wlen, fmt, r, g_brush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        /* fmt cached */
    }
    free(ws);
    return dxgfx_auto_end();
}

extern "C" DXGFX_API int dxgfx_draw_rect(float x, float y, float w, float h, dx_u32 argb, float stroke, int fill) {
    if (stroke <= 0.0f) stroke = 1.0f;
    if (!dxgfx_auto_begin(argb)) return 0;
    float sx1, sy1, sx2, sy2;
    world_to_screen(x, y, &sx1, &sy1);
    world_to_screen(x + w, y + h, &sx2, &sy2);
    D2D1_RECT_F r = D2D1::RectF(sx1, sy1, sx2, sy2);
    if (fill) g_rt->FillRectangle(r, g_brush);
    else g_rt->DrawRectangle(r, g_brush, stroke);
    return dxgfx_auto_end();
}

extern "C" DXGFX_API int dxgfx_draw_line(float x1, float y1, float x2, float y2, dx_u32 argb, float stroke) {
    if (stroke <= 0.0f) stroke = 1.0f;
    if (!dxgfx_auto_begin(argb)) return 0;
    float sx1, sy1, sx2, sy2;
    world_to_screen(x1, y1, &sx1, &sy1);
    world_to_screen(x2, y2, &sx2, &sy2);
    g_rt->DrawLine(D2D1::Point2F(sx1, sy1), D2D1::Point2F(sx2, sy2), g_brush, stroke);
    return dxgfx_auto_end();
}

extern "C" DXGFX_API int dxgfx_mouse_f(float out_xy[2]) {
    if (!out_xy || !dxgfx_init()) return 0;
    float mx, my, cx, cy;
    dxgfx_mouse_in_rt(&mx, &my, &cx, &cy);
    (void)cx; (void)cy;
    out_xy[0] = mx;
    out_xy[1] = my;
    return 1;
}

/* Frame-accumulated wheel in notches (1.0 == one legacy WHEEL_DELTA). Fractional for high-res. */
extern "C" DXGFX_API int dxgfx_mouse_wheel_f(float *out_notches) {
    if (!out_notches || !dxgfx_init()) return 0;
    *out_notches = (float)g_wheel_frame / (float)WHEEL_DELTA;
    return 1;
}

extern "C" DXGFX_API int dxgfx_draw_text_screen(int x, int y, dx_u32 argb, float size, const char *utf8, dx_u32 len) {
    if (!utf8) return 0;
    if (size <= 0.0f) size = 20.0f;
    if (!dxgfx_auto_begin(argb)) return 0;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, 0, 0);
    if (wlen <= 0) return dxgfx_auto_end();
    wchar_t *ws = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    if (!ws) return dxgfx_auto_end();
    MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, ws, wlen);
    ws[wlen] = 0;
    IDWriteTextFormat *fmt = dxgfx_text_format(size);
    HRESULT hr = fmt ? S_OK : E_FAIL;
    if (SUCCEEDED(hr)) {
        D2D1_SIZE_F s = g_rt->GetSize();
        float rw, rh; dxgfx_rt_size(&rw, &rh);
        float sx = (rw > 0.0f) ? (float)x * (s.width / rw) : (float)x;
        float sy = (rh > 0.0f) ? (float)y * (s.height / rh) : (float)y;
        D2D1_RECT_F r = D2D1::RectF(sx, sy, s.width, s.height);
        g_rt->DrawText(ws, (UINT32)wlen, fmt, r, g_brush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        /* fmt cached */
    }
    free(ws);
    return dxgfx_auto_end();
}



/* -------------------------------------------------------------------------- */
/* Icon / simple SVG drawing                                                   */
/* icons/<name>.svg is preferred; missing files use geometric glyph fallback.  */
/* -------------------------------------------------------------------------- */

#define DX_ICON_CACHE 64
typedef struct {
    char name[64];
    char *svg;          /* raw svg text, owned */
    int on;
    int has_file;       /* 1 if file loaded (even empty means tried) */
} IconSlot;
static IconSlot g_icons[DX_ICON_CACHE];

static float dx_icon_size_for(float text_size) {
    float s = text_size * 0.95f;
    if (s < 10.0f) s = 10.0f;
    return s;
}

extern "C" DXGFX_API float dxgfx_icon_size(float text_size) {
    return dx_icon_size_for(text_size);
}

static IconSlot *icon_slot(const char *name) {
    if (!name || !name[0]) return 0;
    unsigned h = 2166136261u;
    for (const char *p = name; *p; p++) { h ^= (unsigned char)*p; h *= 16777619u; }
    unsigned idx = h & (DX_ICON_CACHE - 1);
    for (unsigned n = 0; n < DX_ICON_CACHE; n++) {
        unsigned i = (idx + n) & (DX_ICON_CACHE - 1);
        if (g_icons[i].on && strcmp(g_icons[i].name, name) == 0) return &g_icons[i];
        if (!g_icons[i].on) {
            memset(&g_icons[i], 0, sizeof(g_icons[i]));
            strncpy(g_icons[i].name, name, sizeof(g_icons[i].name) - 1);
            g_icons[i].on = 1;
            /* try load icons/<name>.svg */
            char path[160];
            snprintf(path, sizeof(path), "icons/%s.svg", name);
            FILE *f = fopen(path, "rb");
            if (!f) {
                snprintf(path, sizeof(path), "./icons/%s.svg", name);
                f = fopen(path, "rb");
            }
            if (f) {
                fseek(f, 0, SEEK_END);
                long sz = ftell(f);
                fseek(f, 0, SEEK_SET);
                if (sz > 0 && sz < (1 << 20)) {
                    g_icons[i].svg = (char*)malloc((size_t)sz + 1);
                    if (g_icons[i].svg) {
                        fread(g_icons[i].svg, 1, (size_t)sz, f);
                        g_icons[i].svg[sz] = 0;
                        g_icons[i].has_file = 1;
                    }
                }
                fclose(f);
            }
            return &g_icons[i];
        }
    }
    return 0;
}

/* Very small path subset: M/m L/l H/h V/v Z/z and numbers. Absolute after M. */
static int svg_parse_num(const char **pp, float *out) {
    const char *p = *pp;
    while (*p == ' ' || *p == ',' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    char *end = 0;
    float v = strtof(p, &end);
    if (end == p) return 0;
    *out = v;
    *pp = end;
    return 1;
}

/* Returns 1 if a meaningful glyph was drawn, 0 if name has no known mark.
 * IMPORTANT: do NOT draw a default empty rectangle for unknown names — that
 * made every token look like it had a hollow box to the right of the label.
 */
static int icon_draw_fallback(float x, float y, float size, dx_u32 argb, const char *name) {
    if (!name || !name[0]) return 0;
    float pad = size * 0.18f;
    float x0 = x + pad, y0 = y + pad, x1 = x + size - pad, y1 = y + size - pad;
    float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f;
    float stroke = size * 0.08f;
    if (stroke < 1.0f) stroke = 1.0f;
    if (!strcmp(name, "add") || !strcmp(name, "f32_add")) {
        dxgfx_draw_line(cx, y0, cx, y1, argb, stroke);
        dxgfx_draw_line(x0, cy, x1, cy, argb, stroke);
        return 1;
    }
    if (!strcmp(name, "sub") || !strcmp(name, "f32_sub")) {
        dxgfx_draw_line(x0, cy, x1, cy, argb, stroke);
        return 1;
    }
    if (!strcmp(name, "mul") || !strcmp(name, "f32_mul")) {
        dxgfx_draw_line(x0, y0, x1, y1, argb, stroke);
        dxgfx_draw_line(x1, y0, x0, y1, argb, stroke);
        return 1;
    }
    if (!strcmp(name, "div") || !strcmp(name, "f32_div")) {
        dxgfx_draw_line(x0, y1, x1, y0, argb, stroke);
        dxgfx_draw_rect(cx - stroke, y0 + pad * 0.5f, stroke * 2, stroke * 2, argb, 1, 1);
        dxgfx_draw_rect(cx - stroke, y1 - pad * 0.5f - stroke * 2, stroke * 2, stroke * 2, argb, 1, 1);
        return 1;
    }
    if (!strncmp(name, "var_set", 7)) {
        dxgfx_draw_rect(x0, y0, x1 - x0, y1 - y0, argb, stroke, 0);
        dxgfx_draw_rect(cx - size * 0.12f, cy - size * 0.12f, size * 0.24f, size * 0.24f, argb, 1, 1);
        return 1;
    }
    if (!strncmp(name, "var_read", 8)) {
        dxgfx_draw_line(x0, cy, x1, cy, argb, stroke);
        dxgfx_draw_line(x1 - pad, cy - pad, x1, cy, argb, stroke);
        dxgfx_draw_line(x1 - pad, cy + pad, x1, cy, argb, stroke);
        return 1;
    }
    if (!strncmp(name, "var_write", 9)) {
        dxgfx_draw_line(x0, cy, x1, cy, argb, stroke);
        dxgfx_draw_line(x0, cy, x0 + pad, cy - pad, argb, stroke);
        dxgfx_draw_line(x0, cy, x0 + pad, cy + pad, argb, stroke);
        return 1;
    }
    if (!strcmp(name, "const_payload") || !strcmp(name, "f32_const")) {
        dxgfx_draw_rect(cx - size * 0.18f, cy - size * 0.18f, size * 0.36f, size * 0.36f, argb, stroke, 0);
        return 1;
    }
    if (!strncmp(name, "key_", 4)) {
        dxgfx_draw_rect(x0, cy - size * 0.12f, x1 - x0, size * 0.24f, argb, stroke, 0);
        return 1;
    }
    if (!strcmp(name, "cond_payload") || !strcmp(name, "cond")) {
        dxgfx_draw_line(cx, y0, x1, cy, argb, stroke);
        dxgfx_draw_line(x1, cy, cx, y1, argb, stroke);
        dxgfx_draw_line(cx, y1, x0, cy, argb, stroke);
        dxgfx_draw_line(x0, cy, cx, y0, argb, stroke);
        return 1;
    }
    if (!strcmp(name, "exec") || !strcmp(name, "exec_payload") || !strcmp(name, "jump_payload")) {
        dxgfx_draw_line(x0 + pad * 0.5f, y0, x0 + pad * 0.5f, y1, argb, stroke);
        dxgfx_draw_line(x0 + pad * 0.5f, y0, x1, cy, argb, stroke);
        dxgfx_draw_line(x0 + pad * 0.5f, y1, x1, cy, argb, stroke);
        return 1;
    }
    if (!strcmp(name, "measure_text")) {
        dxgfx_draw_line(x0, y1, x1, y1, argb, stroke);
        dxgfx_draw_line(x0, y0, x0, y1, argb, stroke);
        return 1;
    }
    /* unknown: draw nothing (no hollow box) */
    return 0;
}

static void icon_draw_svg_paths(const char *svg, float x, float y, float size, dx_u32 argb) {
    /* Find viewBox="minx miny w h" else assume 0 0 24 24 */
    float vb_x = 0, vb_y = 0, vb_w = 24, vb_h = 24;
    const char *vb = strstr(svg, "viewBox");
    if (vb) {
        const char *q = strchr(vb, '"');
        if (!q) q = strchr(vb, '\'');
        if (q) {
            q++;
            svg_parse_num(&q, &vb_x);
            svg_parse_num(&q, &vb_y);
            svg_parse_num(&q, &vb_w);
            svg_parse_num(&q, &vb_h);
            if (vb_w < 1) vb_w = 24;
            if (vb_h < 1) vb_h = 24;
        }
    }
    float sx = size / vb_w;
    float sy = size / vb_h;
    const char *p = svg;
    for (;;) {
        const char *dtag = strstr(p, " d=\"");
        const char *dtag2 = strstr(p, " d='");
        const char *dtag3 = strstr(p, "d=\"");
        const char *use = 0;
        char endc = '"';
        if (dtag && (!dtag2 || dtag < dtag2) && (!dtag3 || dtag <= dtag3)) { use = dtag + 4; endc = '"'; }
        else if (dtag2 && (!dtag3 || dtag2 <= dtag3)) { use = dtag2 + 4; endc = '\''; }
        else if (dtag3) { use = dtag3 + 3; endc = '"'; }
        else break;
        const char *dend = strchr(use, endc);
        if (!dend) break;
        /* parse path */
        const char *q = use;
        float cx = 0, cy = 0, sx0 = 0, sy0 = 0;
        int have = 0;
        char cmd = 'M';
        while (q < dend) {
            while (q < dend && (*q == ' ' || *q == ',' || *q == '\n' || *q == '\r' || *q == '\t')) q++;
            if (q >= dend) break;
            if ((*q >= 'A' && *q <= 'Z') || (*q >= 'a' && *q <= 'z')) { cmd = *q++; continue; }
            float n1 = 0, n2 = 0;
            if (cmd == 'H' || cmd == 'h' || cmd == 'V' || cmd == 'v') {
                if (!svg_parse_num(&q, &n1)) break;
            } else if (cmd == 'Z' || cmd == 'z') {
                if (have) {
                    float x1 = x + (sx0 - vb_x) * sx, y1 = y + (sy0 - vb_y) * sy;
                    float x2 = x + (cx - vb_x) * sx, y2 = y + (cy - vb_y) * sy;
                    dxgfx_draw_line(x2, y2, x1, y1, argb, 1.5f);
                }
                continue;
            } else {
                if (!svg_parse_num(&q, &n1)) break;
                if (!svg_parse_num(&q, &n2)) break;
            }
            float nx = cx, ny = cy;
            switch (cmd) {
            case 'M': nx = n1; ny = n2; cmd = 'L'; sx0 = nx; sy0 = ny; have = 0; break;
            case 'm': nx = cx + n1; ny = cy + n2; cmd = 'l'; sx0 = nx; sy0 = ny; have = 0; break;
            case 'L': nx = n1; ny = n2; break;
            case 'l': nx = cx + n1; ny = cy + n2; break;
            case 'H': nx = n1; ny = cy; break;
            case 'h': nx = cx + n1; ny = cy; break;
            case 'V': nx = cx; ny = n1; break;
            case 'v': nx = cx; ny = cy + n1; break;
            case 'C': {
                /* cubic: n1,n2 already first ctrl; need 4 more numbers */
                float x2, y2, x3, y3;
                if (!svg_parse_num(&q, &x2) || !svg_parse_num(&q, &y2) || !svg_parse_num(&q, &x3) || !svg_parse_num(&q, &y3)) break;
                /* approximate with polyline */
                float p0x = cx, p0y = cy, p1x = n1, p1y = n2, p2x = x2, p2y = y2, p3x = x3, p3y = y3;
                float px = p0x, py = p0y;
                for (int i = 1; i <= 8; i++) {
                    float t = i / 8.0f, u = 1.0f - t;
                    float bx = u*u*u*p0x + 3*u*u*t*p1x + 3*u*t*t*p2x + t*t*t*p3x;
                    float by = u*u*u*p0y + 3*u*u*t*p1y + 3*u*t*t*p2y + t*t*t*p3y;
                    float ax = x + (px - vb_x) * sx, ay = y + (py - vb_y) * sy;
                    float bx2 = x + (bx - vb_x) * sx, by2 = y + (by - vb_y) * sy;
                    dxgfx_draw_line(ax, ay, bx2, by2, argb, 1.5f);
                    px = bx; py = by;
                }
                cx = p3x; cy = p3y; have = 1;
                continue;
            }
            case 'c': {
                float x2, y2, x3, y3;
                if (!svg_parse_num(&q, &x2) || !svg_parse_num(&q, &y2) || !svg_parse_num(&q, &x3) || !svg_parse_num(&q, &y3)) break;
                float p0x = cx, p0y = cy, p1x = cx + n1, p1y = cy + n2, p2x = cx + x2, p2y = cy + y2, p3x = cx + x3, p3y = cy + y3;
                float px = p0x, py = p0y;
                for (int i = 1; i <= 8; i++) {
                    float t = i / 8.0f, u = 1.0f - t;
                    float bx = u*u*u*p0x + 3*u*u*t*p1x + 3*u*t*t*p2x + t*t*t*p3x;
                    float by = u*u*u*p0y + 3*u*u*t*p1y + 3*u*t*t*p2y + t*t*t*p3y;
                    float ax = x + (px - vb_x) * sx, ay = y + (py - vb_y) * sy;
                    float bx2 = x + (bx - vb_x) * sx, by2 = y + (by - vb_y) * sy;
                    dxgfx_draw_line(ax, ay, bx2, by2, argb, 1.5f);
                    px = bx; py = by;
                }
                cx = p3x; cy = p3y; have = 1;
                continue;
            }
            default:
                /* skip unknown by consuming nothing more */
                break;
            }
            if (have) {
                float ax = x + (cx - vb_x) * sx, ay = y + (cy - vb_y) * sy;
                float bx = x + (nx - vb_x) * sx, by = y + (ny - vb_y) * sy;
                dxgfx_draw_line(ax, ay, bx, by, argb, 1.5f);
            }
            cx = nx; cy = ny; have = 1;
        }
        p = dend + 1;
    }
}

extern "C" DXGFX_API int dxgfx_draw_icon(float x, float y, float size, dx_u32 argb, const char *name) {
    /* Returns 1 if an SVG or known glyph was drawn; 0 if nothing (no placeholder box). */
    if (!name || size <= 0.0f) return 0;
    if (!dxgfx_init()) return 0;
    IconSlot *slot = icon_slot(name);
    if (slot && slot->has_file && slot->svg && slot->svg[0]) {
        icon_draw_svg_paths(slot->svg, x, y, size, argb);
        return 1;
    }
    return icon_draw_fallback(x, y, size, argb, name);
}

/* Non-drawing probe: 1 if icons/<name>.svg exists or a built-in glyph is known. */
extern "C" DXGFX_API int dxgfx_has_icon(const char *name) {
    if (!name || !name[0]) return 0;
    IconSlot *slot = icon_slot(name);
    if (slot && slot->has_file && slot->svg && slot->svg[0]) return 1;
    /* same names as icon_draw_fallback */
    if (!strcmp(name, "add") || !strcmp(name, "f32_add")) return 1;
    if (!strcmp(name, "sub") || !strcmp(name, "f32_sub")) return 1;
    if (!strcmp(name, "mul") || !strcmp(name, "f32_mul")) return 1;
    if (!strcmp(name, "div") || !strcmp(name, "f32_div")) return 1;
    if (!strncmp(name, "var_set", 7) || !strncmp(name, "var_read", 8) || !strncmp(name, "var_write", 9)) return 1;
    if (!strcmp(name, "const_payload") || !strcmp(name, "f32_const")) return 1;
    if (!strncmp(name, "key_", 4)) return 1;
    if (!strcmp(name, "cond_payload") || !strcmp(name, "cond")) return 1;
    if (!strcmp(name, "exec") || !strcmp(name, "exec_payload") || !strcmp(name, "jump_payload")) return 1;
    if (!strcmp(name, "measure_text")) return 1;
    return 0;
}
