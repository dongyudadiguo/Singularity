#include <windows.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;

#define STACK_CAP (1u << 20)

static u8 *stk;
static u32 sp;

static void ensure_stack(void) {
    if (!stk) {
        stk = (u8*)malloc(STACK_CAP);
        sp = 0;
    }
}

__declspec(dllexport) void *pop(u32 size) {
    ensure_stack();
    if (size > sp) {
        sp = 0;
        return stk;
    }
    sp -= size;
    return stk + sp;
}

__declspec(dllexport) void push(const void *p, u32 size) {
    ensure_stack();
    if (size > STACK_CAP) size = STACK_CAP;
    if (sp + size > STACK_CAP) sp = 0;
    memcpy(stk + sp, p, size);
    sp += size;
}

__declspec(dllexport) u32 cvm_stack_size(void) {
    ensure_stack();
    return sp;
}

__declspec(dllexport) void cvm_stack_clear(void) {
    ensure_stack();
    sp = 0;
}
