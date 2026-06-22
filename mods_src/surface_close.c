#include "surface_ops.h"
__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    surface_context_reset();
    if (s) {
        if (s->surface_hwnd && IsWindow(s->surface_hwnd)) DestroyWindow(s->surface_hwnd);
        s->surface_hwnd = 0;
        s->surface_event = 0;
        s->surface_x = 0;
        s->surface_y = 0;
    }
    cnext();
}
