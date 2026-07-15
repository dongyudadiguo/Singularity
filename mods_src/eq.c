#include <string.h>
typedef unsigned u32;

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);

__declspec(dllexport) void run(void) {
    int b = *(int*)from(4);
    int a = *(int*)from(4);
    u32 r = (a == b) ? 1 : 0;
    memcpy(slot(4), &r, 4);
    cont();
}
