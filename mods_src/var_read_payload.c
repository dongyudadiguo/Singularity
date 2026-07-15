#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *slot(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size);

/* payload: entire payload is the variable id (any size). */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (!n) { cont(); return; }
    u32 size = 0;
    u8 *data = cvm_var_get(p, n, &size);
    if (data) memcpy(slot(size), data, size);
    cont();
}
