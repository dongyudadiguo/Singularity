typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void *p, u32 size);

#include "../dxgfx.h"

__declspec(dllexport) void run(void) {
    int state[4] = {0, 0, 0, 0};
    dxgfx_mouse(state);
    push(state, sizeof(state));
    cont();
}
