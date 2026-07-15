#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
/* stack: text[count] (bottom ... top: text then nothing else / pop text last)
 * payload: f32 size, u32 count
 * pushes f32 width; measures up to first NUL within count.
 */
__declspec(dllexport) void run(void) {
    float w = 0.0f;
    u8 *p = cvm_payload();
    if (cvm_payload_size() >= 8) {
        float size = *(float *)p;
        u32 count = *(u32 *)(p + 4);
        char *s = (char *)from(count ? count : 1);
        u32 z = 0;
        while (z < count && s[z]) z++;
        if (z) {
            float out[2] = {0.0f, 0.0f};
            if (dxgfx_measure_text(size, s, z, out)) w = out[0];
        }
    }
    memcpy(slot(4), &w, 4);
    cont();
}
