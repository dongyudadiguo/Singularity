typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) void cvm_var_write(const u8 *id, u32 id_len, const u8 *data, u32 size);

/* stack: id_len[u32], id[id_len], data[var_size]  (pop order: data, id, id_len) */
__declspec(dllexport) void run(void) {
    u32 id_len = *(u32*)pop(4);
    if (id_len > 256) { cont(); return; }
    u8 *id = (u8*)pop(id_len ? id_len : 1);
    if (!id_len) { cont(); return; }
    u32 vsize = 0;
    if (!cvm_var_get(id, id_len, &vsize)) { cont(); return; }
    u8 *data = (u8*)pop(vsize ? vsize : 1);
    cvm_var_write(id, id_len, data, vsize);
    cont();
}
