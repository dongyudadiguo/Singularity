
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <stdint.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

static HWND g_hwnd;
static ID2D1Factory *g_d2d;
static IDWriteFactory *g_dw;
static ID2D1HwndRenderTarget *g_rt;
static ID2D1SolidColorBrush *g_brush;

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) { return 0; }
    return DefWindowProcA(h, m, w, l);
}

static D2D1_COLOR_F color_from_argb(uint32_t c) {
    D2D1_COLOR_F r;
    r.a = ((c >> 24) & 255) / 255.0f;
    r.r = ((c >> 16) & 255) / 255.0f;
    r.g = ((c >> 8) & 255) / 255.0f;
    r.b = (c & 255) / 255.0f;
    return r;
}

extern "C" __declspec(dllexport) int dxgfx_ensure(void) { return 1; }
