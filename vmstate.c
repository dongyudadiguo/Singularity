#include <windows.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

__declspec(dllexport) u8 *ptr;
static u8 *cur_base;
static H cur_key;

extern __declspec(dllimport) void cvm_cache_pin_base(const u8 *base);
extern __declspec(dllimport) void cvm_cache_unpin_base(const u8 *base);

typedef struct Frame {
    u8 *base;
    u8 *ret;
    H key;
} Frame;

static Frame frames[1024];
static u32 frame_sp;

__declspec(dllexport) u32 cvm_token_size(void) {
    if (!ptr) return 0;
    return *(u32 *)ptr;
}
__declspec(dllexport) u8 *cvm_token(void) {
    if (!ptr) return 0;
    return ptr + 4;
}
__declspec(dllexport) u32 cvm_payload_size(void) {
    u32 tlen;
    if (!ptr) return 0;
    tlen = *(u32 *)ptr;
    return *(u32 *)(ptr + 4 + tlen);
}
__declspec(dllexport) u8 *cvm_payload(void) {
    u32 tlen;
    if (!ptr) return 0;
    tlen = *(u32 *)ptr;
    return ptr + 8 + tlen;
}
__declspec(dllexport) u8 *cvm_current_base(void) { return cur_base; }
__declspec(dllexport) u8 *cvm_current_key(void) { return cur_key; }
__declspec(dllexport) void cvm_restart_current(void) { ptr = cur_base; }

__declspec(dllexport) void cvm_replace_current(const H k, u8 *base) {
    for (u32 i = 0; i < frame_sp; i++) cvm_cache_unpin_base(frames[i].base);
    if (cur_base) cvm_cache_unpin_base(cur_base);
    frame_sp = 0;
    if (k) memcpy(cur_key, k, 32);
    cur_base = base;
    ptr = base;
    if (cur_base) cvm_cache_pin_base(cur_base);
}

__declspec(dllexport) void cvm_set_current(const H k, u8 *base) {
    if (cur_base && frame_sp < (u32)(sizeof(frames) / sizeof(frames[0]))) {
        frames[frame_sp].base = cur_base;
        frames[frame_sp].ret = ptr;
        memcpy(frames[frame_sp].key, cur_key, 32);
        frame_sp++;
    }
    if (k) memcpy(cur_key, k, 32);
    cur_base = base;
    ptr = base;
    if (cur_base) cvm_cache_pin_base(cur_base);
}

__declspec(dllexport) int cvm_ret(void) {
    if (!frame_sp) return 0;
    if (cur_base) cvm_cache_unpin_base(cur_base);
    frame_sp--;
    cur_base = frames[frame_sp].base;
    ptr = frames[frame_sp].ret;
    memcpy(cur_key, frames[frame_sp].key, 32);
    return 1;
}

/* instr helpers for editors */
__declspec(dllexport) u32 cvm_instr_size_at(const u8 *p) {
    u32 tlen, plen;
    if (!p) return 0;
    tlen = *(u32 *)p;
    if (tlen == 0) return 4;
    plen = *(u32 *)(p + 4 + tlen);
    return 8 + tlen + plen;
}
