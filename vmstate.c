#include <windows.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

__declspec(dllexport) u8 *ptr;
static u8 *cur_base;
static H cur_key;

/* Provided by vmstore: protect live instruction streams from LRU eviction. */
extern __declspec(dllimport) void cvm_cache_pin_base(const u8 *base);
extern __declspec(dllimport) void cvm_cache_unpin_base(const u8 *base);

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

__declspec(dllexport) void cvm_replace_current(const H k, u8 *base) {
    /* Drop call stack pins. */
    for (u32 i = 0; i < frame_sp; i++) cvm_cache_unpin_base(frames[i].base);
    if (cur_base) cvm_cache_unpin_base(cur_base);
    frame_sp = 0;
    if (k) memcpy(cur_key, k, 32);
    cur_base = base;
    ptr = base;
    if (cur_base) cvm_cache_pin_base(cur_base);
}

__declspec(dllexport) void cvm_set_current(const H k, u8 *base) {
    /*
     * Entering a resolved block replaces the current instruction stream.
     * Save the caller state first. Parent stays pinned; child is pinned too.
     */
    if (cur_base && frame_sp < (u32)(sizeof(frames) / sizeof(frames[0]))) {
        frames[frame_sp].base = cur_base;
        frames[frame_sp].ret = ptr;
        memcpy(frames[frame_sp].key, cur_key, 32);
        frame_sp++;
        /* parent already pinned from when it was entered */
    }
    if (k) memcpy(cur_key, k, 32);
    cur_base = base;
    ptr = base;
    if (cur_base) cvm_cache_pin_base(cur_base);
}

__declspec(dllexport) int cvm_ret(void) {
    if (!frame_sp) return 0;
    /* Leaving child block: unpin its stream. */
    if (cur_base) cvm_cache_unpin_base(cur_base);
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
