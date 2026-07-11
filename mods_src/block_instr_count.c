#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void push(const void*,u32);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);
static int zero32(const u8*p){for(int i=0;i<32;i++)if(p[i])return 0;return 1;}
/* current cached block -> u32 instruction count (end index) */
__declspec(dllexport) void run(void){
    u8 *b=cvm_cached_base(); u32 n=cvm_cached_len(), o=0, r=0;
    while (o+36<=n && !zero32(b+o)) {
        u32 pn=*(u32*)(b+o+32);
        if (o+36+pn>n) break;
        o += 36+pn; r++; if (r>256) break;
    }
    push(&r,4); cont();
}
