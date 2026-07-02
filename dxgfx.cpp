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
static const int G_W = 1280;
static const int G_H = 720;

static LRESULT CALLBACK dxgfx_wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE) { ShowWindow(h, SW_HIDE); return 0; }
    if (m == WM_DESTROY) return 0;
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
    g_hwnd = CreateWindowExA(0, wc.lpszClassName, "Singularity DirectX", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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

static int dxgfx_begin(dx_u32 argb) {
    if (!dxgfx_init()) return 0;
    if (!IsWindowVisible(g_hwnd)) ShowWindow(g_hwnd, SW_SHOW);
    dxgfx_pump();
    g_rt->BeginDraw();
    g_brush->SetColor(dxgfx_color(argb));
    return 1;
}

static int dxgfx_end(void) {
    HRESULT hr = g_rt->EndDraw();
    dxgfx_pump();
    return SUCCEEDED(hr);
}

extern "C" DXGFX_API int dxgfx_keyboard(dx_u8 out_state[256]) {
    if (!out_state) return 0;
    dxgfx_pump();
    for (int i = 0; i < 256; i++) {
        SHORT a = GetAsyncKeyState(i);
        SHORT k = GetKeyState(i);
        out_state[i] = (dx_u8)(((a & 0x8000) ? 0x80 : 0) | ((k & 1) ? 0x01 : 0));
    }
    return 1;
}

extern "C" DXGFX_API int dxgfx_mouse(int out_state[4]) {
    if (!out_state) return 0;
    dxgfx_pump();
    POINT p;
    GetCursorPos(&p);
    out_state[0] = (int)p.x;
    out_state[1] = (int)p.y;
    out_state[2] = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0) |
                   ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 2 : 0) |
                   ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 4 : 0) |
                   ((GetAsyncKeyState(VK_XBUTTON1) & 0x8000) ? 8 : 0) |
                   ((GetAsyncKeyState(VK_XBUTTON2) & 0x8000) ? 16 : 0);
    out_state[3] = 0;
    return 1;
}

extern "C" DXGFX_API int dxgfx_draw_text(int x, int y, dx_u32 argb, float size, const char *utf8, dx_u32 len) {
    if (!utf8) return 0;
    if (size <= 0.0f) size = 20.0f;
    if (!dxgfx_begin(argb)) return 0;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, 0, 0);
    if (wlen <= 0) { dxgfx_end(); return 0; }
    wchar_t *ws = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    if (!ws) { dxgfx_end(); return 0; }
    MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, ws, wlen);
    ws[wlen] = 0;

    IDWriteTextFormat *fmt = 0;
    HRESULT hr = g_dw->CreateTextFormat(L"Segoe UI", 0, DWRITE_FONT_WEIGHT_NORMAL,
                                        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                        size, L"", &fmt);
    if (SUCCEEDED(hr)) {
        D2D1_SIZE_F s = g_rt->GetSize();
        D2D1_RECT_F r = D2D1::RectF((FLOAT)x, (FLOAT)y, s.width, s.height);
        g_rt->DrawText(ws, (UINT32)wlen, fmt, r, g_brush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        fmt->Release();
    }
    free(ws);
    return dxgfx_end();
}

extern "C" DXGFX_API int dxgfx_draw_rect(float x, float y, float w, float h, dx_u32 argb, float stroke, int fill) {
    if (stroke <= 0.0f) stroke = 1.0f;
    if (!dxgfx_begin(argb)) return 0;
    D2D1_RECT_F r = D2D1::RectF(x, y, x + w, y + h);
    if (fill) g_rt->FillRectangle(r, g_brush);
    else g_rt->DrawRectangle(r, g_brush, stroke);
    return dxgfx_end();
}

extern "C" DXGFX_API int dxgfx_draw_line(float x1, float y1, float x2, float y2, dx_u32 argb, float stroke) {
    if (stroke <= 0.0f) stroke = 1.0f;
    if (!dxgfx_begin(argb)) return 0;
    g_rt->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), g_brush, stroke);
    return dxgfx_end();
}
