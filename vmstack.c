#include <windows.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;

/*
 * Stack = linear byte buffer + mark frames.
 *
 *   mark()       save sp (return point)
 *   back()       restore sp to last mark
 *   slot(n)      grow by n, return writable ptr at old top  (was push region)
 *   from(n)      shrink by n, return ptr to removed bytes   (was pop)
 *
 * No push/pop exports.
 */

#define STACK_CAP (1u << 20)
#define MARK_CAP  4096

static u8 *stk;
static u32 sp;
static u32 marks[MARK_CAP];
static u32 mark_sp;

static void ensure_stack(void) {
    if (!stk) {
        stk = (u8 *)malloc(STACK_CAP);
        sp = 0;
        mark_sp = 0;
    }
}

__declspec(dllexport) void mark(void) {
    ensure_stack();
    if (mark_sp < MARK_CAP) marks[mark_sp++] = sp;
}

__declspec(dllexport) void back(void) {
    ensure_stack();
    if (!mark_sp) { sp = 0; return; }
    sp = marks[--mark_sp];
    if (sp > STACK_CAP) sp = 0;
}

__declspec(dllexport) void *slot(u32 size) {
    void *p;
    ensure_stack();
    if (size > STACK_CAP) size = STACK_CAP;
    if (sp + size > STACK_CAP) sp = 0;
    p = stk + sp;
    sp += size;
    return p;
}

__declspec(dllexport) void *from(u32 size) {
    ensure_stack();
    if (size > sp) {
        sp = 0;
        return stk;
    }
    sp -= size;
    return stk + sp;
}

__declspec(dllexport) u32 cvm_stack_size(void) {
    ensure_stack();
    return sp;
}

__declspec(dllexport) u32 cvm_mark_depth(void) {
    ensure_stack();
    return mark_sp;
}

__declspec(dllexport) void cvm_stack_clear(void) {
    ensure_stack();
    sp = 0;
    mark_sp = 0;
}
