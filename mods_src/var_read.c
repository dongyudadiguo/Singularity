typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 *size);

__declspec(dllexport) void run(void) {
    H id;
    u8 *p = pop(32);
    for (u32 i = 0; i < 32; i++) id[i] = p[i];
    u32 size;
    u8 *data = cvm_var_get(id, &size);
    if (data) push(data, size);
    cont();
}
