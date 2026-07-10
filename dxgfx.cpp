#define WIN32_LEAN_AND_MEAN
#define DXGFX_BUILD
#include "dxgfx.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <stdlib.h>
#include <string.h>

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
static int g_wheel = 0;
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
    if (m == WM_MOUSEWHEEL) { g_wheel += GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA; return 0; }
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
    /* Clear edge-like inputs after the frame that consumed them. */
    g_wheel = 0;
    /* Drain OS queue now so next frame_begin starts with fresher state. */
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
    out_state[3] = g_wheel;
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
        mouse[5] = g_wheel;
        mouse[6] = 0;
        mouse[7] = 0;
    }
    g_prev_mouse_buttons = mb;
    /* g_wheel is cleared in dxgfx_frame_end so all peeks in a frame see the same delta. */
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
