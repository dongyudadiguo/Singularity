typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
/* stack: f32 x1 y1 x2 y2 ; payload: u32 ARGB, f32 stroke (or defaults) */
__declspec(dllexport) void run(void) {
    float y2 = *(float*)pop(4);
    float x2 = *(float*)pop(4);
    float y1 = *(float*)pop(4);
    float x1 = *(float*)pop(4);
    u32 argb = 0xff62c982;
    float stroke = 2.0f;
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n >= 4) argb = *(u32*)p;
    if (n >= 8) stroke = *(float*)(p+4);
    dxgfx_draw_line(x1, y1, x2, y2, argb, stroke);
    cont();
}
