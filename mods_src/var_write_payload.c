typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) void cvm_var_set(const u8 *id, u32 id_len, u32 size);
extern __declspec(dllimport) void cvm_var_write(const u8 *id, u32 id_len, const u8 *data, u32 size);

/* payload:
 *   id_len[u32] + id[id_len]                 -> pop current var size, write
 *   id_len[u32] + id[id_len] + data[...]     -> set+write data
 * Legacy:
 *   id[32] / id[32]+data
 */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n < 4) { cont(); return; }

    u32 id_len = *(u32*)p;
    if (id_len > 0 && id_len <= 256 && n >= 4 + id_len) {
        const u8 *id = p + 4;
        u32 rest = n - 4 - id_len;
        if (rest > 0) {
            cvm_var_set(id, id_len, rest);
            cvm_var_write(id, id_len, p + 4 + id_len, rest);
        } else {
            u32 vsize = 0;
            if (!cvm_var_get(id, id_len, &vsize)) { cont(); return; }
            u8 *data = (u8*)from(vsize ? vsize : 1);
            cvm_var_write(id, id_len, data, vsize);
        }
        cont();
        return;
    }

    if (n < 32) { cont(); return; }
    const u8 *id = p;
    if (n > 32) {
        u32 size = n - 32;
        cvm_var_set(id, 32, size);
        cvm_var_write(id, 32, p + 32, size);
    } else {
        u32 vsize = 0;
        if (!cvm_var_get(id, 32, &vsize)) { cont(); return; }
        u8 *data = (u8*)from(vsize ? vsize : 1);
        cvm_var_write(id, 32, data, vsize);
    }
    cont();
}
