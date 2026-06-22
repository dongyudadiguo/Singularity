#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    H rect;
    cvm_pop(rect);
    surface_clip_push_rect(rect);
    cnext();
}
