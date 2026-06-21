#include "surface_ops.h"
__declspec(dllexport) void run(void) { H color; cvm_pop(color); HWND w=surface_hwnd(); if(w){RECT r; GetClientRect(w,&r); HDC dc=GetDC(w); HBRUSH br=CreateSolidBrush((COLORREF)cvm_h_to_u64(color)); FillRect(dc,&r,br); DeleteObject(br); ReleaseDC(w,dc);} cnext(); }
