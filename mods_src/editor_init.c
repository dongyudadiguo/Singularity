typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) int cvm_sha256(const u8 *p, u32 n, H out);
extern __declspec(dllimport) u32 cvm_children(const H parent, H *out, u32 cap);
__declspec(dllexport) void run(void) {
    H tag, kids[8];
    const char s[] = "#TAG";
    cvm_sha256((const u8*)s, 4, tag);
    cvm_children(tag, kids, 8);
    cont();
}
