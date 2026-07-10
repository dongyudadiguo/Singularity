typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include <windows.h>
/* payload: u32 mask (1=L,2=R,4=M). push u32 0/1 if any masked button down. */
static u8 down(int vk){ return (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0; }
__declspec(dllexport) void run(void) {
    u32 mask = cvm_payload_size() >= 4 ? *(u32*)cvm_payload() : 1u;
    u32 bits = 0;
    if (down(VK_LBUTTON)) bits |= 1u;
    if (down(VK_RBUTTON)) bits |= 2u;
    if (down(VK_MBUTTON)) bits |= 4u;
    u32 v = (bits & mask) ? 1u : 0u;
    push(&v, 4);
    cont();
}
