typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void cvm_var_set(const u8 *id, u32 id_len, u32 size);

/* stack: id_len[u32], id[id_len], size[u32]  (pop order: size, id, id_len) */
__declspec(dllexport) void run(void) {
    u32 size = *(u32*)pop(4);
    u32 id_len = *(u32*)pop(4);
    if (id_len > 256) { cont(); return; }
    u8 *id = (u8*)pop(id_len ? id_len : 1);
    if (!id_len) { cont(); return; }
    cvm_var_set(id, id_len, size);
    cont();
}
