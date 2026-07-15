#include <string.h>
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *slot(u32 size);

#include "../dxgfx.h"

__declspec(dllexport) void run(void) {
    dx_u8 state[256];
    for (u32 i = 0; i < 256; i++) state[i] = 0;
    dxgfx_keyboard(state);
    memcpy(slot(256), state, 256);
    cont();
}
