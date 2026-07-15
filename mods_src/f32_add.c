#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
/* stack: a b -> a+b */
__declspec(dllexport) void run(void) {
    float b = *(float*)from(4);
    float a = *(float*)from(4);
    float r = a + b;
    memcpy(slot(4), &r, 4);
    cont();
}
