#include <windows.h>

typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) u8 *ptr;

__declspec(dllexport) void *pop(u32 size) {
    ptr -= size;
    return ptr;
}

__declspec(dllexport) void push(const void *p, u32 size) {
    for (u32 i = 0; i < size; i++) ptr[i] = ((const u8*)p)[i];
    ptr += size;
}
