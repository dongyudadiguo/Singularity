#include <windows.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

__declspec(dllexport) u8 *ptr;
static u8 *cur_base;
static H cur_key;

__declspec(dllexport) u8 *cvm_token(void) { return ptr; }
__declspec(dllexport) u8 *cvm_payload(void) { return ptr + 36; }
__declspec(dllexport) u32 cvm_payload_size(void) { return *(u32*)(ptr + 32); }
__declspec(dllexport) u8 *cvm_current_base(void) { return cur_base; }
__declspec(dllexport) u8 *cvm_current_key(void) { return cur_key; }

__declspec(dllexport) void cvm_set_current(const H k, u8 *base) {
    if (k) memcpy(cur_key, k, 32);
    cur_base = base;
    ptr = base;
}

__declspec(dllexport) void cvm_advance(H next) {
    ptr += 32 + *(u32*)(ptr + 32);
    memcpy(next, ptr, 32);
}
