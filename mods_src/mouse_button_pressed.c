typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include <windows.h>
/* payload: u32 mask bit (1=L,2=R,4=M). Only the lowest set bit is tested/updated. */
static u8 prev_l, prev_r, prev_m;
static u8 down(int vk){ return (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0; }
__declspec(dllexport) void run(void) {
    u32 mask = cvm_payload_size() >= 4 ? *(u32*)cvm_payload() : 1u;
    u32 bit = mask & (1u | 2u | 4u);
    if (bit & (bit - 1u)) bit &= ~(bit - 1u); /* lowest bit only */
    u32 v = 0;
    if (bit == 1u) {
        u8 d = down(VK_LBUTTON); v = (d && !prev_l) ? 1u : 0u; prev_l = d;
    } else if (bit == 2u) {
        u8 d = down(VK_RBUTTON); v = (d && !prev_r) ? 1u : 0u; prev_r = d;
    } else if (bit == 4u) {
        u8 d = down(VK_MBUTTON); v = (d && !prev_m) ? 1u : 0u; prev_m = d;
    }
    push(&v, 4);
    cont();
}
