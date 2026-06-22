#include "surface_ops.h"

__declspec(dllexport) void run(void) {
    H x_h, y_h;
    cvm_pop(y_h);
    cvm_pop(x_h);
    surface_translate_push_xy((LONG)cvm_h_to_u64(x_h), (LONG)cvm_h_to_u64(y_h));
    cnext();
}
