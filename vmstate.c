#include <windows.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

__declspec(dllexport) u8 *ptr;
static u8 *cur_base;
static H cur_key;

typedef struct Frame {
    u8 *base;
    u8 *ret;
    H key;
} Frame;

static Frame frames[1024];
static u32 frame_sp;

__declspec(dllexport) u8 *cvm_token(void) { return ptr; }
__declspec(dllexport) u8 *cvm_payload(void) { return ptr + 36; }
__declspec(dllexport) u32 cvm_payload_size(void) { return *(u32*)(ptr + 32); }
__declspec(dllexport) u8 *cvm_current_base(void) { return cur_base; }
__declspec(dllexport) u8 *cvm_current_key(void) { return cur_key; }
__declspec(dllexport) void cvm_restart_current(void) { ptr = cur_base; }

__declspec(dllexport) void cvm_set_current(const H k, u8 *base) {
    /*
     * Entering a resolved block replaces the current instruction stream.
     * Save the caller state first; ptr already points at the instruction after
     * the token that entered this block, so it is the return point for ret.
     */
    if (cur_base && frame_sp < (u32)(sizeof(frames) / sizeof(frames[0]))) {
        frames[frame_sp].base = cur_base;
        frames[frame_sp].ret = ptr;
        memcpy(frames[frame_sp].key, cur_key, 32);
        frame_sp++;
    }
    if (k) memcpy(cur_key, k, 32);
    cur_base = base;
    ptr = base;
}

__declspec(dllexport) int cvm_ret(void) {
    if (!frame_sp) return 0;
    frame_sp--;
    cur_base = frames[frame_sp].base;
    ptr = frames[frame_sp].ret;
    memcpy(cur_key, frames[frame_sp].key, 32);
    return 1;
}

__declspec(dllexport) void cvm_advance(H next) {
    memcpy(next, ptr, 32);
    ptr += 32 + 4 + *(u32*)(ptr + 32);
}
