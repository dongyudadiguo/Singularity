#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);

/* stack: id_len[u32], id[id_len]  (pop order: id, id_len) */
__declspec(dllexport) void run(void) {
    u32 id_len = *(u32*)from(4);
    if (id_len > 256) { cont(); return; }
    u8 *id = (u8*)from(id_len ? id_len : 1);
    if (!id_len) { cont(); return; }
    u32 size = 0;
    u8 *data = cvm_var_get(id, id_len, &size);
    if (data) memcpy(slot(size), data, size);
    cont();
}
