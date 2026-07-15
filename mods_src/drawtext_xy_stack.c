typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
/* stack: f32 x, f32 y, then text bytes[count]
 * payload: u32 ARGB, f32 size, u32 byte_count
 * draws at (int)x,(int)y
 */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    if (cvm_payload_size() < 12) { cont(); return; }
    u32 argb = *(u32*)p;
    float size = *(float*)(p+4);
    u32 count = *(u32*)(p+8);
    char *s = (char*)from(count);
    float y = *(float*)from(4);
    float x = *(float*)from(4);
    dxgfx_draw_text((int)x, (int)y, argb, size, s, count);
    cont();
}
