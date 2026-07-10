typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
/* stack: f32 x, f32 y, text[count]; payload: u32 ARGB, f32 size, u32 count
 * Draws only up to first NUL within count (match labels are C strings in a fixed buffer).
 */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    if (cvm_payload_size() >= 12) {
        u32 argb = *(u32 *)p;
        float size = *(float *)(p + 4);
        u32 count = *(u32 *)(p + 8);
        char *s = (char *)pop(count);
        float y = *(float *)pop(4);
        float x = *(float *)pop(4);
        u32 z = 0;
        while (z < count && s[z]) z++;
        if (z) dxgfx_draw_text_screen((int)x, (int)y, argb, size, s, z);
    }
    cont();
}
