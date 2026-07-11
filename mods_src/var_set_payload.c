typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) void cvm_var_set(const u8 *id, u32 id_len, u32 size);
extern __declspec(dllimport) void cvm_var_write(const u8 *id, u32 id_len, const u8 *data, u32 size);

/* payload (arbitrary-size id):
 *   id_len[u32] + id[id_len] + size[u32]           -> allocate zeroed var (tail exactly 4)
 *   id_len[u32] + id[id_len] + initial_data[...]    -> allocate/write data (tail != 4, may be 0)
 *
 * Legacy fallback (no id_len header):
 *   id[32] + size[u32]                              -> n == 36
 *   id[32] + data[...]                              -> n > 36 or n == 32
 */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n < 4) { cont(); return; }

    u32 id_len = *(u32*)p;
    if (id_len > 0 && id_len <= 256 && n >= 4 + id_len) {
        const u8 *id = p + 4;
        u32 rest = n - 4 - id_len;
        if (rest == 4) {
            u32 size = *(u32*)(p + 4 + id_len);
            cvm_var_set(id, id_len, size);
        } else {
            cvm_var_set(id, id_len, rest);
            if (rest) cvm_var_write(id, id_len, p + 4 + id_len, rest);
        }
        cont();
        return;
    }

    /* legacy fixed-32 */
    if (n < 32) { cont(); return; }
    const u8 *id = p;
    if (n == 36) {
        u32 size = *(u32*)(p + 32);
        cvm_var_set(id, 32, size);
    } else {
        u32 size = n - 32;
        cvm_var_set(id, 32, size);
        if (size) cvm_var_write(id, 32, p + 32, size);
    }
    cont();
}
