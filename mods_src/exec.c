typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void cvm_exec(const H h);

static int zero32(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

/* Unconditional exec of a stack token.
 * Target may be native (DLL) or a logical/content block key; VM decides.
 * Stack: ... token[32] -> ...
 */
__declspec(dllexport) void run(void) {
    H h;
    u8 *p = (u8*)pop(32);
    for (u32 i = 0; i < 32; i++) h[i] = p[i];
    if (zero32(h)) { cont(); return; }
    cvm_exec(h);
}
