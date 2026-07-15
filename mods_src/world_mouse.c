#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *slot(u32 size);
#include "../dxgfx.h"
/* push f32 world_x, f32 world_y */
__declspec(dllexport) void run(void) {
    float xy[2] = {0.0f, 0.0f};
    dxgfx_world_mouse(xy);
    memcpy(slot(sizeof(xy)), xy, sizeof(xy));
    cont();
}
