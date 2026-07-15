#include <string.h>
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);

__declspec(dllexport) void run(void) {
    u32 b = *(u32*)from(4);
    u32 a = *(u32*)from(4);
    u32 r = a * b;
    memcpy(slot(4), &r, 4);
    cont();
}
