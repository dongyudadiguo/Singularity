#include "mod.h"
#include "../dxgfx.h"

__declspec(dllexport) void run(void) {
    int state[4] = {0, 0, 0, 0};
    dxgfx_mouse(state);
    push(state, sizeof(state));
    cont();
}
