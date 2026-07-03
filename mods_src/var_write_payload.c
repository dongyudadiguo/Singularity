typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 *size);
extern __declspec(dllimport) void cvm_var_set(const u8 *id, u32 size);
extern __declspec(dllimport) void cvm_var_write(const u8 *id, const u8 *data, u32 size);

/* payload:
 *   id[32]                 -> pop current variable size bytes and write
 *   id[32] + data[...]     -> write arbitrary payload bytes, resizing/creating var
 */
__declspec(dllexport) void run(void) {
    H id;
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n < 32) { cont(); return; }
    for (u32 i = 0; i < 32; i++) id[i] = p[i];
    if (n > 32) {
        u32 size = n - 32;
        cvm_var_set(id, size);
        cvm_var_write(id, p + 32, size);
    } else {
        u32 vsize;
        if (!cvm_var_get(id, &vsize)) { cont(); return; }
        u8 *data = pop(vsize);
        cvm_var_write(id, data, vsize);
    }
    cont();
}
