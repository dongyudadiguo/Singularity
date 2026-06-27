#ifndef MOD_H
#define MOD_H

typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) u8 *ptr;
extern __declspec(dllimport) void cont(void);

static inline void *pop(void) {
    u32 n = *(u32*)(ptr - 4);
    ptr -= 4 + n;
    return ptr;
}

static inline void push(const void *p, u32 size) {
    *(u32*)ptr = size;
    ptr += 4;
    for (u32 i = 0; i < size; i++) ptr[i] = ((const u8*)p)[i];
    ptr += size;
}

#endif
