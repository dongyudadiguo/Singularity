#include "name_common.h"
typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32);
extern __declspec(dllimport) void push(const void*, u32);
/* stack: name[96], query[256] -> u32 0/1  (pop query first) */
__declspec(dllexport) void run(void){
    char *q = (char*)pop(256);
    char *n = (char*)pop(96);
    u32 r = (n && q && str_prefix_ci_us(n, q)) ? 1u : 0u;
    push(&r, 4);
    cont();
}
