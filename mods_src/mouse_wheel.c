#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *slot(u32 size);
#include "../dxgfx.h"
/* push f32 wheel notches for this frame (1.0 == one legacy detent).
 * High-res / precision trackpads produce fractional values; do not quantize to i32. */
__declspec(dllexport) void run(void) {
    float notches = 0.0f;
    dxgfx_mouse_wheel_f(&notches);
    memcpy(slot(4), &notches, 4);
    cont();
}
