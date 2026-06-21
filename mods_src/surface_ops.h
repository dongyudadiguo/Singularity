#ifndef MOD_SURFACE_OPS_H
#define MOD_SURFACE_OPS_H

#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"

static LRESULT CALLBACK surface_proc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    CvmState *s = cvm_state();
    if (s) {
        if (m == WM_LBUTTONDOWN || m == WM_MOUSEMOVE || m == WM_LBUTTONUP) {
            s->surface_event = m;
            s->surface_x = (u64)(short)LOWORD(lp);
            s->surface_y = (u64)(short)HIWORD(lp);
        } else if (m == WM_KEYDOWN) {
            s->surface_event = 0x10000 | (u64)wp;
        } else if (m == WM_SIZE) {
            s->surface_w = (u64)LOWORD(lp);
            s->surface_h = (u64)HIWORD(lp);
        } else if (m == WM_CLOSE) {
            s->surface_event = 0xffffffff;
            ShowWindow(w, SW_HIDE);
            return 0;
        }
    }
    return DefWindowProcA(w, m, wp, lp);
}

static void surface_class(void) {
    static int done;
    if (done) return;
    done = 1;
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = surface_proc;
    wc.hInstance = GetModuleHandleA(0);
    wc.lpszClassName = "CVM_Surface";
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);
}

static HWND surface_hwnd(void) {
    CvmState *s = cvm_state();
    return s ? s->surface_hwnd : 0;
}

static void surface_rect_from_h(H h, RECT *r) {
    H v;
    cvm_zero(v); memcpy(v, h, 8); r->left = (LONG)cvm_h_to_u64(v);
    cvm_zero(v); memcpy(v, h + 8, 8); r->top = (LONG)cvm_h_to_u64(v);
    cvm_zero(v); memcpy(v, h + 16, 8); r->right = r->left + (LONG)cvm_h_to_u64(v);
    cvm_zero(v); memcpy(v, h + 24, 8); r->bottom = r->top + (LONG)cvm_h_to_u64(v);
}

#endif
