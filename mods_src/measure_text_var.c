#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const H, u32 *);
#include "../dxgfx.h"
/* payload: id[32] + f32 size
 * measures var C string width in layout units (pre-zoom).
 * pushes f32 width.
 */
__declspec(dllexport) void run(void) {
    float w = 0.0f;
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n >= 36) {
        H id; memcpy(id, p, 32);
        float size = *(float *)(p + 32);
        u32 vn = 0;
        u8 *s = cvm_var_get(id, &vn);
        if (s && vn) {
            u32 z = 0;
            while (z < vn && s[z]) z++;
            float out[2] = {0.0f, 0.0f};
            if (z && dxgfx_measure_text(size, (const char *)s, z, out)) w = out[0];
        }
    }
    push(&w, 4);
    cont();
}
