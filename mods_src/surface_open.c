#include "surface_ops.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    H w_h, h_h, o;
    cvm_pop(h_h);
    cvm_pop(w_h);
    cvm_zero(o);
    surface_class();
    if (s) {
        int w = (int)cvm_h_to_u64(w_h), h = (int)cvm_h_to_u64(h_h);
        if (w < 1) w = 800;
        if (h < 1) h = 600;
        if (s->surface_hwnd && !IsWindow(s->surface_hwnd)) s->surface_hwnd = 0;
        s->surface_event = 0;
        s->surface_x = 0;
        s->surface_y = 0;
        if (!s->surface_hwnd) {
            surface_context_reset();
            s->surface_hwnd = CreateWindowW(L"CVM_Surface", L"CVM Toy", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, w, h, 0, 0, GetModuleHandleW(0), 0);
        }
        if (s->surface_hwnd && IsWindow(s->surface_hwnd)) {
            ShowWindow(s->surface_hwnd, SW_SHOW);
            UpdateWindow(s->surface_hwnd);
            RECT r;
            GetClientRect(s->surface_hwnd, &r);
            s->surface_w = (u64)(r.right - r.left);
            s->surface_h = (u64)(r.bottom - r.top);
            o[0] = 1;
        } else {
            s->surface_hwnd = 0;
        }
    }
    cvm_push(o);
    cnext();
}
