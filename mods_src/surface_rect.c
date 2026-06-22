#include "surface_ops.h"
__declspec(dllexport) void run(void) { H rect,color; cvm_pop(color); cvm_pop(rect); HWND w=surface_hwnd(); if(w){RECT r; surface_rect_from_h(rect,&r); HDC dc=GetDC(w); surface_apply_clip(dc); HBRUSH br=CreateSolidBrush((COLORREF)cvm_h_to_u64(color)); FillRect(dc,&r,br); DeleteObject(br); ReleaseDC(w,dc);} cnext(); }
