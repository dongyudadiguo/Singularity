typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cvm_advance(H next);
extern __declspec(dllimport) void cvm_exec(const H h);

__declspec(dllexport) void cont(void) {
    H next;
    cvm_advance(next);
    cvm_exec(next);
}
