typedef unsigned char u8;
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
/* payload: raw f32 bytes (4). push that float. */
__declspec(dllexport) void run(void) {
    if (cvm_payload_size() >= 4) push(cvm_payload(), 4);
    cont();
}
