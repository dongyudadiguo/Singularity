typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
/* stack: f32 x y w h ; payload: u32 ARGB, f32 stroke, u32 fill */
__declspec(dllexport) void run(void) {
    float h = *(float*)pop(4);
    float w = *(float*)pop(4);
    float y = *(float*)pop(4);
    float x = *(float*)pop(4);
    u32 argb = 0xff34414d;
    float stroke = 1.0f;
    int fill = 1;
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n >= 4) argb = *(u32*)p;
    if (n >= 8) stroke = *(float*)(p+4);
    if (n >= 12) fill = *(u32*)(p+8) ? 1 : 0;
    dxgfx_draw_rect(x, y, w, h, argb, stroke, fill);
    cont();
}
