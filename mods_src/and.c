#include "mod.h"

__declspec(dllexport) void run(void) {
    int b = mod_bool(pop(4));
    int a = mod_bool(pop(4));
    u32 r = (a && b) ? 1 : 0;
    push(&r, 4);
    cont();
}
