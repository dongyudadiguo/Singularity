typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void cvm_reexec(void);

static int mod_bool(const void *p) {
    const u8 *b = (const u8*)p;
    for (u32 i = 0; i < 4; i++) if (b[i]) return 1;
    return 0;
}

__declspec(dllexport) void run(void) {
    if (mod_bool(pop(4))) cvm_reexec();
    else cont();
}
