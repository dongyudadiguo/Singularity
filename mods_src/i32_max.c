#include <string.h>
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) void *slot(u32 size);
/* stack a b -> max(a,b) as i32 */
__declspec(dllexport) void run(void) {
    int b = *(int*)from(4);
    int a = *(int*)from(4);
    int r = a > b ? a : b;
    memcpy(slot(4), &r, 4);
    cont();
}
