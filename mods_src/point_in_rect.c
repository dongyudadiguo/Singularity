#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
/* stack: x y rx ry rw rh  (all f32) -> u32 0/1
 * hit if x in [rx,rx+rw) and y in [ry,ry+rh)
 */
__declspec(dllexport) void run(void) {
    float rh = *(float*)from(4);
    float rw = *(float*)from(4);
    float ry = *(float*)from(4);
    float rx = *(float*)from(4);
    float y  = *(float*)from(4);
    float x  = *(float*)from(4);
    u32 v = (x >= rx && x < rx + rw && y >= ry && y < ry + rh) ? 1u : 0u;
    memcpy(slot(4), &v, 4);
    cont();
}
