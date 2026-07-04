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
static ID2D1SolidColorBrush *g_brush = 0;
static int g_inited = 0;
static int g_drawing = 0;
static int g_should_close = 0;
static int g_wheel = 0;
static int g_text_len = 0;
static dx_u32 g_text[64];
static dx_u8 g_prev[256];
static dx_u8 g_now[256];
static float g_cam_x = 0.0f;
static float g_cam_y = 0.0f;
static float g_cam_zoom = 1.0f;
static const int G_W = 1280;
static const int G_H = 720;

static LRESULT CALLBACK dxgfx_wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE) { g_should_close = 1; ShowWindow(h, SW_HIDE); return 0; }
    if (m == WM_DESTROY) { g_should_close = 1; return 0; }
    if (m == WM_MOUSEWHEEL) { g_wheel += GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA; return 0; }
    if (m == WM_CHAR) {
        if (w >= 32 && g_text_len < (int)(sizeof(g_text) / sizeof(g_text[0]))) g_text[g_text_len++] = (dx_u32)w;
        return 0;
    }
    return DefWindowProcA(h, m, w, l);
}

static void dxgfx_sample_keys(void) {
    memcpy(g_prev, g_now, sizeof(g_now));
    for (int i = 0; i < 256; i++) {
        SHORT a = GetAsyncKeyState(i);
        SHORT k = GetKeyState(i);
        g_now[i] = (dx_u8)(((a & 0x8000) ? 0x80 : 0) | ((k & 1) ? 0x01 : 0));
    }
}

static void dxgfx_pump(void) {
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    dxgfx_sample_keys();
}

static D2D1_COLOR_F dxgfx_color(dx_u32 c) {
    D2D1_COLOR_F r;
    r.a = ((c >> 24) & 255) / 255.0f;
    r.r = ((c >> 16) & 255) / 255.0f;
    r.g = ((c >> 8) & 255) / 255.0f;
    r.b = (c & 255) / 255.0f;
    return r;
}

static D2D1_POINT_2F world_point(float x, float y) {
    return D2D1::Point2F((x - g_cam_x) * g_cam_zoom, (y - g_cam_y) * g_cam_zoom);
}

static D2D1_RECT_F world_rect(float x, float y, float w, float h) {
    D2D1_POINT_2F a = world_point(x, y);
    return D2D1::RectF(a.x, a.y, a.x + w * g_cam_zoom, a.y + h * g_cam_zoom);
}

static int dxgfx_init(void) {
    if (g_inited) { dxgfx_pump(); return g_rt != 0; }
    g_inited = 1;

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
    g_hwnd = CreateWindowExA(0, wc.lpszClassName, "Singularity", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                             0, 0, inst, 0);
    if (!g_hwnd) return 0;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2d))) return 0;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_dw))) return 0;

    RECT cr;
    GetClientRect(g_hwnd, &cr);
    D2D1_SIZE_U sz = D2D1::SizeU((UINT32)(cr.right - cr.left), (UINT32)(cr.bottom - cr.top));
    D2D1_HWND_RENDER_TARGET_PROPERTIES hp = D2D1::HwndRenderTargetProperties(g_hwnd, sz, D2D1_PRESENT_OPTIONS_NONE);
    D2D1_RENDER_TARGET_PROPERTIES rp = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_HARDWARE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        0.0f, 0.0f, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);
    if (FAILED(g_d2d->CreateHwndRenderTarget(rp, hp, &g_rt))) return 0;
    if (FAILED(g_rt->CreateSolidColorBrush(dxgfx_color(0xffffffff), &g_brush))) return 0;
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    dxgfx_pump();
    return 1;
}

static int dxgfx_ensure_begin(void) {
    if (!dxgfx_init()) return 0;
    if (!IsWindowVisible(g_hwnd) && !g_should_close) ShowWindow(g_hwnd, SW_SHOW);
    if (!g_drawing) { g_rt->BeginDraw(); g_drawing = 1; }
    return 1;
}

extern "C" DXGFX_API int dxgfx_frame_begin(void) {
    g_text_len = 0;
    dxgfx_pump();
    return dxgfx_ensure_begin();
}

extern "C" DXGFX_API int dxgfx_clear(dx_u32 argb) {
    if (!dxgfx_ensure_begin()) return 0;
    g_rt->Clear(dxgfx_color(argb));
    return 1;
}

extern "C" DXGFX_API int dxgfx_frame_end(void) {
    if (!g_drawing || !g_rt) return 1;
    HRESULT hr = g_rt->EndDraw();
    g_drawing = 0;
    dxgfx_pump();
    return SUCCEEDED(hr);
}

extern "C" DXGFX_API int dxgfx_keyboard(dx_u8 out_state[256]) {
    if (!out_state) return 0;
    dxgfx_pump();
    memcpy(out_state, g_now, 256);
    return 1;
}

extern "C" DXGFX_API int dxgfx_mouse(int out_state[4]) {
    if (!out_state) return 0;
    dxgfx_pump();
    POINT p;
    GetCursorPos(&p);
    if (g_hwnd) ScreenToClient(g_hwnd, &p);
    out_state[0] = (int)p.x;
    out_state[1] = (int)p.y;
    out_state[2] = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0) |
                   ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 2 : 0) |
                   ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 4 : 0) |
                   ((GetAsyncKeyState(VK_XBUTTON1) & 0x8000) ? 8 : 0) |
                   ((GetAsyncKeyState(VK_XBUTTON2) & 0x8000) ? 16 : 0);
    out_state[3] = g_wheel;
    g_wheel = 0;
    return 1;
}

extern "C" DXGFX_API int dxgfx_screen_size(int out_size[2]) {
    if (!out_size || !dxgfx_init()) return 0;
    RECT cr;
    GetClientRect(g_hwnd, &cr);
    out_size[0] = cr.right - cr.left;
    out_size[1] = cr.bottom - cr.top;
    return 1;
}

extern "C" DXGFX_API int dxgfx_window_should_close(void) { dxgfx_pump(); return g_should_close; }

extern "C" DXGFX_API int dxgfx_set_camera(float x, float y, float zoom) {
    g_cam_x = x;
    g_cam_y = y;
    g_cam_zoom = zoom <= 0.01f ? 0.01f : zoom;
    return 1;
}

extern "C" DXGFX_API int dxgfx_world_mouse(float out_xy[2]) {
    if (!out_xy) return 0;
    int m[4] = {0};
    dxgfx_mouse(m);
    out_xy[0] = (float)m[0] / g_cam_zoom + g_cam_x;
    out_xy[1] = (float)m[1] / g_cam_zoom + g_cam_y;
    return 1;
}

extern "C" DXGFX_API int dxgfx_mouse_wheel(void) {
    int m[4] = {0};
    dxgfx_mouse(m);
    return m[3];
}

extern "C" DXGFX_API int dxgfx_key_state(int vk, int kind) {
    dxgfx_pump();
    if (vk < 0 || vk >= 256) return 0;
    int now = (g_now[vk] & 0x80) ? 1 : 0;
    int prev = (g_prev[vk] & 0x80) ? 1 : 0;
    if (kind == 1) return now && !prev;
    if (kind == 2) return !now && prev;
    return now;
}

extern "C" DXGFX_API int dxgfx_text_input(dx_u32 *out_codepoint) {
    if (!out_codepoint) return 0;
    dxgfx_pump();
    if (g_text_len <= 0) return 0;
    *out_codepoint = g_text[0];
    memmove(g_text, g_text + 1, (g_text_len - 1) * sizeof(g_text[0]));
    g_text_len--;
    return 1;
}

extern "C" DXGFX_API int dxgfx_draw_text(int x, int y, dx_u32 argb, float size, const char *utf8, dx_u32 len) {
    if (!utf8) return 0;
    if (size <= 0.0f) size = 20.0f;
    int own = !g_drawing;
    if (!dxgfx_ensure_begin()) return 0;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, 0, 0);
    if (wlen <= 0) { if (own) dxgfx_frame_end(); return 0; }
    wchar_t *ws = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    if (!ws) { if (own) dxgfx_frame_end(); return 0; }
    MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, ws, wlen);
    ws[wlen] = 0;

    IDWriteTextFormat *fmt = 0;
    HRESULT hr = g_dw->CreateTextFormat(L"Segoe UI", 0, DWRITE_FONT_WEIGHT_NORMAL,
                                        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                        size * g_cam_zoom, L"", &fmt);
    if (SUCCEEDED(hr)) {
        D2D1_SIZE_F s = g_rt->GetSize();
        D2D1_POINT_2F pt = world_point((float)x, (float)y);
        D2D1_RECT_F r = D2D1::RectF(pt.x, pt.y, s.width, s.height);
        g_brush->SetColor(dxgfx_color(argb));
        g_rt->DrawText(ws, (UINT32)wlen, fmt, r, g_brush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        fmt->Release();
    }
    free(ws);
    return own ? dxgfx_frame_end() : 1;
}

extern "C" DXGFX_API int dxgfx_draw_rect(float x, float y, float w, float h, dx_u32 argb, float stroke, int fill) {
    if (stroke <= 0.0f) stroke = 1.0f;
    int own = !g_drawing;
    if (!dxgfx_ensure_begin()) return 0;
    D2D1_RECT_F r = world_rect(x, y, w, h);
    g_brush->SetColor(dxgfx_color(argb));
    if (fill) g_rt->FillRectangle(r, g_brush);
    else g_rt->DrawRectangle(r, g_brush, stroke * g_cam_zoom);
    return own ? dxgfx_frame_end() : 1;
}

extern "C" DXGFX_API int dxgfx_draw_line(float x1, float y1, float x2, float y2, dx_u32 argb, float stroke) {
    if (stroke <= 0.0f) stroke = 1.0f;
    int own = !g_drawing;
    if (!dxgfx_ensure_begin()) return 0;
    D2D1_POINT_2F a = world_point(x1, y1);
    D2D1_POINT_2F b = world_point(x2, y2);
    g_brush->SetColor(dxgfx_color(argb));
    g_rt->DrawLine(a, b, g_brush, stroke * g_cam_zoom);
    return own ? dxgfx_frame_end() : 1;
}
