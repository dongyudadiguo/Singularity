typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
#include "../dxgfx.h"
/* stack: cam_x cam_y zoom (f32 each, zoom on top) */
__declspec(dllexport) void run(void) {
    float zoom = *(float*)pop(4);
    float y = *(float*)pop(4);
    float x = *(float*)pop(4);
    dxgfx_set_camera(x, y, zoom);
    cont();
}
