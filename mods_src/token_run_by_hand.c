#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) void cvm_exec(const H h);
extern __declspec(dllimport) void cvm_heat_pulse(u32 uid, const H node_key);
extern __declspec(dllimport) int cvm_hand_armed(u32 uid);

/*
 * payload: uid[u32] + token[32]  (36 bytes)
 * If uid is armed (process-local), exec token and heat target.
 * Arm toggled from editor link button; lost on process restart.
 */
__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    u32 uid;
    H tok;
    if (n < 36) { cont(); return; }
    uid = *(u32 *)p;
    memcpy(tok, p + 4, 32);
    if (!cvm_hand_armed(uid)) { cont(); return; }
    cvm_heat_pulse(uid ? uid : 1, tok);
    cvm_exec(tok);
}
