typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) void cvm_exec(const H h);

static int mod_bool(const void *p) {
    const u8 *b = (const u8*)p;
    for (u32 i = 0; i < 4; i++) if (b[i]) return 1;
    return 0;
}

/*
 * payload: token[32]
 * stack:   bool
 *
 * If true, exec token without rewriting the live instruction payload.
 * (cvm_exec_payload mutates payload key->content hash and can corrupt the
 * currently executing program block under reexec; that breaks later frames
 * and can async-write a bad program override.)
 */
__declspec(dllexport) void run(void) {
    H h;
    int ok = mod_bool(pop(4));
    u8 *p = cvm_payload();
    if (cvm_payload_size() < 32) { cont(); return; }
    for (u32 i = 0; i < 32; i++) h[i] = p[i];
    if (ok) cvm_exec(h);
    else cont();
}
