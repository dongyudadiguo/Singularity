#include "mod.h"

__declspec(dllexport) void run(void) {
    int b = *(int*)pop(4);
    int a = *(int*)pop(4);
    u32 r = (a > b) ? 1 : 0;
    push(&r, 4);
    cont();
}
