#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8*, u32, u32*);
extern __declspec(dllimport) void cvm_var_write(const u8*, u32, const u8*, u32);
/* payload: entire payload is variable id (any size) */
__declspec(dllexport) void run(void){
    u8 *add = (u8*)pop(256);
    u8 *id = cvm_payload();
    u32 id_len = cvm_payload_size();
    if (!id_len) { cont(); return; }
    u32 n = 0;
    u8 *s = cvm_var_get(id, id_len, &n);
    if (s && n) {
        u32 a = 0; while (a < n && s[a]) a++;
        u32 z = 0; while (z < 256 && add[z]) z++;
        if (z > n - 1 - a) z = n - 1 - a;
        if (z) { memcpy(s + a, add, z); s[a + z] = 0; cvm_var_write(id, id_len, s, n); }
    }
    cont();
}
