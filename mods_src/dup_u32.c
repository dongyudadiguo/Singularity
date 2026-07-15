#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
__declspec(dllexport) void run(void) {
    u32 v = *(u32*)from(4);
    memcpy(slot(4), &v, 4);
    memcpy(slot(4), &v, 4);
    cont();
}
