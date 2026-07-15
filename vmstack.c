#include <windows.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;

/*
 * Stack model (mark / back):
 *   mark()  — push current sp as a return point
 *   back()  — restore sp to last mark (pop the frame)
 *   push/pop remain for byte payloads between marks (compat)
 *
 * Frame stack is independent of data stack so nested scopes work.
 */

#define STACK_CAP (1u << 20)
#define MARK_CAP  4096

static u8 *stk;
static u32 sp;
static u32 marks[MARK_CAP];
static u32 mark_sp;

static void ensure_stack(void) {
    if (!stk) {
        stk = (u8*)malloc(STACK_CAP);
        sp = 0;
        mark_sp = 0;
    }
}

/* Record return point = current sp. */
__declspec(dllexport) void mark(void) {
    ensure_stack();
    if (mark_sp < MARK_CAP) marks[mark_sp++] = sp;
}

/* Restore sp to last mark (discard data above it). */
__declspec(dllexport) void back(void) {
    ensure_stack();
    if (!mark_sp) { sp = 0; return; }
    sp = marks[--mark_sp];
    if (sp > STACK_CAP) sp = 0;
}

/* Optional: back without removing mark (peek restore) — not exported. */

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
    if (p && size) memcpy(stk + sp, p, size);
    sp += size;
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
