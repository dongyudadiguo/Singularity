typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
/* payload: s32 x, s32 y, u32 ARGB, f32 size, text... (screen space) */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n >= 16) {
        dxgfx_draw_text_screen(*(int*)p, *(int*)(p+4), *(u32*)(p+8), *(float*)(p+12),
                               (const char*)(p+16), n-16);
    }
    cont();
}
