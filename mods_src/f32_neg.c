#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
__declspec(dllexport) void run(void) {
    float x = *(float*)from(4);
    x = -x;
    memcpy(slot(4), &x, 4);
    cont();
}
