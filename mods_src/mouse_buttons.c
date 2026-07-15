#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
__declspec(dllexport) void run(void) {
    int *m = (int*)from(16);
    int b = m[2];
    memcpy(slot(4), &b, 4);
    cont();
}
