typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
__declspec(dllexport) void run(void) {
    u32 vk = 0, r = 0;
    if (cvm_payload_size() >= 4) vk = *(u32*)cvm_payload();
    u8 *state = (u8*)pop(256);
    if (vk < 256 && (state[vk] & 0x80)) r = 1;
    push(&r, 4);
    cont();
}
