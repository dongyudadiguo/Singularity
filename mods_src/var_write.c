typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 *size);
extern __declspec(dllimport) void cvm_var_write(const u8 *id, const u8 *data, u32 size);

__declspec(dllexport) void run(void) {
    H id;
    u8 *p = pop(32);
    for (u32 i = 0; i < 32; i++) id[i] = p[i];
    u32 vsize;
    if (!cvm_var_get(id, &vsize)) { cont(); return; }
    u8 *data = pop(vsize);
    cvm_var_write(id, data, vsize);
    cont();
}
