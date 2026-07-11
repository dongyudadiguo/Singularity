typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8*, u32, u32*);
#include "../dxgfx.h"
/* stack: f32 x, f32 y; payload: id[32], u32 ARGB, f32 size */
__declspec(dllexport) void run(void) {
    float y = *(float*)pop(4);
    float x = *(float*)pop(4);
    u8 *p = cvm_payload();
    if (cvm_payload_size() >= 40) {
        u32 n = 0;
        u8 *s = cvm_var_get(p, 32, &n);
        if (s) {
            u32 z = 0; while (z < n && s[z]) z++;
            dxgfx_draw_text_screen((int)x, (int)y, *(u32*)(p+32), *(float*)(p+36), (char*)s, z);
        }
    }
    cont();
}
