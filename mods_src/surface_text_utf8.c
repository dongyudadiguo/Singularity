#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    H text_h, x_h, y_h, color;
    cvm_pop(color);
    cvm_pop(y_h);
    cvm_pop(x_h);
    cvm_pop(text_h);
    HWND w = surface_hwnd();
    u32 len = 0;
    u8 *d = block_read(text_h, &len);
    if (w && d && len) {
        int wide_len = MultiByteToWideChar(CP_UTF8, 0, (char*)d, (int)len, 0, 0);
        if (wide_len > 0) {
            WCHAR *wide = (WCHAR*)malloc(sizeof(WCHAR) * (u32)wide_len);
            if (wide) {
                MultiByteToWideChar(CP_UTF8, 0, (char*)d, (int)len, wide, wide_len);
                HDC dc = GetDC(w);
                surface_apply_clip(dc);
                SetTextColor(dc, (COLORREF)cvm_h_to_u64(color));
                SetBkMode(dc, TRANSPARENT);
                TextOutW(dc, (int)surface_coord_x(x_h), (int)surface_coord_y(y_h), wide, wide_len);
                ReleaseDC(w, dc);
                free(wide);
            }
        }
    }
    if (d) free(d);
    cnext();
}
