#include <windows.h>

typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) u8 *ptr;

__declspec(dllexport) void *pop(void) {
    u32 n = *(u32*)(ptr - 4);
    ptr -= 4 + n;
    return ptr;
}

__declspec(dllexport) void push(const void *p, u32 size) {
    *(u32*)ptr = size;
    ptr += 4;
    for (u32 i = 0; i < size; i++) ptr[i] = ((const u8*)p)[i];
    ptr += size;
}
