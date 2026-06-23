#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    H x1_h, y1_h, x2_h, y2_h, color;
    cvm_pop(color);
    cvm_pop(y2_h);
    cvm_pop(x2_h);
    cvm_pop(y1_h);
    cvm_pop(x1_h);
    HWND w = surface_hwnd();
    if (w) {
        HDC dc = GetDC(w);
        surface_apply_clip(dc);
        HPEN pen = CreatePen(PS_SOLID, 1, (COLORREF)cvm_h_to_u64(color));
        HGDIOBJ old = SelectObject(dc, pen);
        MoveToEx(dc, surface_coord_x(x1_h), surface_coord_y(y1_h), 0);
        LineTo(dc, surface_coord_x(x2_h), surface_coord_y(y2_h));
        SelectObject(dc, old);
        DeleteObject(pen);
        ReleaseDC(w, dc);
    }
    cnext();
}
