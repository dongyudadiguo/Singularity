typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) void cvm_exec_payload(H h);

static int zero32(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

/* Unconditional exec of payload token[32].
 * Same dual-identity resolution as cond_payload success path.
 * Payload: token[32]
 */
__declspec(dllexport) void run(void) {
    H h;
    u8 *p = cvm_payload();
    if (cvm_payload_size() < 32) { cont(); return; }
    for (u32 i = 0; i < 32; i++) h[i] = p[i];
    if (zero32(h)) { cont(); return; }
    cvm_exec_payload(h);
}
