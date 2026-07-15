typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void cvm_exec(const H h);

static int mod_bool(const void *p) {
    const u8 *b = (const u8*)p;
    for (u32 i = 0; i < 4; i++) if (b[i]) return 1;
    return 0;
}

__declspec(dllexport) void run(void) {
    H h;
    int ok = mod_bool(from(4));
    u8 *p = from(32);
    for (u32 i = 0; i < 32; i++) h[i] = p[i];
    if (ok) cvm_exec(h);
    else cont();
}
