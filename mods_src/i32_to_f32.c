#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
__declspec(dllexport) void run(void) {
    int v = *(int*)from(4);
    float f = (float)v;
    memcpy(slot(4), &f, 4);
    cont();
}
