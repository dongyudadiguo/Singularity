#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    H x_h, y_h, scale_h;
    cvm_pop(scale_h);
    cvm_pop(y_h);
    cvm_pop(x_h);
    surface_camera_push_xy_scale((LONG)cvm_h_to_u64(x_h), (LONG)cvm_h_to_u64(y_h), (LONG)cvm_h_to_u64(scale_h));
    cnext();
}
