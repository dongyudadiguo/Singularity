#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *slot(u32 size);
#include "../dxgfx.h"
__declspec(dllexport) void run(void) {
    int state[4] = {0,0,0,0};
    dxgfx_mouse(state);
    int x = state[0];
    memcpy(slot(4), &x, 4);
    cont();
}
