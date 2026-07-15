#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *slot(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);
extern __declspec(dllimport) int cvm_sha256(const u8 *p, u32 n, H out);

/* payload: entire payload is var id.
 * Hash the C-string prefix of the var (up to first NUL), else full size.
 * Push token[32] (zero on empty/fail).
 */
__declspec(dllexport) void run(void) {
    H out;
    u8 *id = cvm_payload();
    u32 id_len = cvm_payload_size();
    u32 size = 0;
    u8 *data;
    u32 n = 0;
    for (u32 i = 0; i < 32; i++) out[i] = 0;
    if (!id_len) { memcpy(slot(32), out, 32); cont(); return; }
    data = cvm_var_get(id, id_len, &size);
    if (!data || !size) { memcpy(slot(32), out, 32); cont(); return; }
    while (n < size && data[n]) n++;
    if (!n) { memcpy(slot(32), out, 32); cont(); return; }
    cvm_sha256(data, n, out);
    memcpy(slot(32), out, 32);
    cont();
}
