#include <windows.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cvm_firstchild(H p, H c);
extern __declspec(dllimport) void cvm_exec(const H h);

extern IMAGE_DOS_HEADER __ImageBase;

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int self_token(H out) {
    char path[MAX_PATH];
    char *name;
    int len;
    if (!GetModuleFileNameA((HMODULE)&__ImageBase, path, MAX_PATH)) return 0;
    name = strrchr(path, '\\');
    name = name ? name + 1 : path;
    len = (int)strlen(name);
    if (len < 68) return 0; /* 64 hex + .dll */
    for (int i = 0; i < 32; i++) {
        int hi = hexval(name[i * 2]);
        int lo = hexval(name[i * 2 + 1]);
        if (hi < 0 || lo < 0) return 0;
        out[i] = (u8)((hi << 4) | lo);
    }
    return 1;
}

__declspec(dllexport) void run(void) {
    H self = {0};
    H first = {0};
    if (!self_token(self)) return;
    cvm_firstchild(self, first);
    cvm_exec(first);
}
