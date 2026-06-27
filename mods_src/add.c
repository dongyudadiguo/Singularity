#include "mod.h"

__declspec(dllexport) void run(void) {
    u32 r = *(u32*)pop() + *(u32*)pop();
    push(&r, 4);
    cont();
}
