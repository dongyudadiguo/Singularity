#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);

static int mod_bool(const void *p) {
    const u8 *b = (const u8*)p;
    for (u32 i = 0; i < 4; i++) if (b[i]) return 1;
    return 0;
}

__declspec(dllexport) void run(void) {
    int b = mod_bool(from(4));
    int a = mod_bool(from(4));
    u32 r = (a || b) ? 1 : 0;
    memcpy(slot(4), &r, 4);
    cont();
}
