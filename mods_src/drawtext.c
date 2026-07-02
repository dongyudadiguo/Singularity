#include "mod.h"
#include "../dxgfx.h"

/* payload layout: s32 x, s32 y, u32 ARGB, f32 size, UTF-8 text bytes... */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n >= 16) {
        int x = *(int*)(p + 0);
        int y = *(int*)(p + 4);
        u32 argb = *(u32*)(p + 8);
        float size = *(float*)(p + 12);
        dxgfx_draw_text(x, y, argb, size, (const char*)(p + 16), n - 16);
    }
    cont();
}
