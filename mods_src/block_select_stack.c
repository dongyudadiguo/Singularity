typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H, H);
__declspec(dllexport) void run(void) {
    H key, h;
    u8 *p = (u8*)pop(32);
    for (int i = 0; i < 32; i++) key[i] = p[i];
    cvm_resolve_payload_hash(key, h);
    cont();
}
