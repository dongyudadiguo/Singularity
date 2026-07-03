typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);

#include "../dxgfx.h"

/* payload layout: f32 x, f32 y, f32 w, f32 h, u32 ARGB, f32 stroke, u32 fill */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n >= 28) {
        float x = *(float*)(p + 0);
        float y = *(float*)(p + 4);
        float w = *(float*)(p + 8);
        float h = *(float*)(p + 12);
        u32 argb = *(u32*)(p + 16);
        float stroke = *(float*)(p + 20);
        int fill = *(u32*)(p + 24) ? 1 : 0;
        dxgfx_draw_rect(x, y, w, h, argb, stroke, fill);
    }
    cont();
}
