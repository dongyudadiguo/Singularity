#include "mod.h"
#include "../dxgfx.h"

/* payload layout: f32 x1, f32 y1, f32 x2, f32 y2, u32 ARGB, f32 stroke */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n >= 24) {
        float x1 = *(float*)(p + 0);
        float y1 = *(float*)(p + 4);
        float x2 = *(float*)(p + 8);
        float y2 = *(float*)(p + 12);
        u32 argb = *(u32*)(p + 16);
        float stroke = *(float*)(p + 20);
        dxgfx_draw_line(x1, y1, x2, y2, argb, stroke);
    }
    cont();
}
