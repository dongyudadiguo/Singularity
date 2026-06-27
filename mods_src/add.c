#include "mod.h"

void add(void) {
    u32 r = *(u32*)pop() + *(u32*)pop();
    push(&r, 4);
    cont();
}
