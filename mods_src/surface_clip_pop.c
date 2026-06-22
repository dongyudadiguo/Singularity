#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    surface_clip_pop_rect();
    cnext();
}
