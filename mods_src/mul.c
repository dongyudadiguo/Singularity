#include "mod.h"

__declspec(dllexport) void run(void) {
    u32 b = *(u32*)pop(4);
    u32 a = *(u32*)pop(4);
    u32 r = a * b;
    push(&r, 4);
    cont();
}
