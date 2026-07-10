#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void*pop(u32); extern __declspec(dllimport) u8*cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void); extern __declspec(dllimport) void cvm_cached_set_len(u32);
#define MAX_BLOCK (1u<<20)
__declspec(dllexport) void run(void){u8 token[32];memcpy(token,pop(32),32);u32 off=*(u32*)pop(4),nz=0;for(int i=0;i<32;i++)nz|=token[i];u8*b=cvm_cached_base();u32 n=cvm_cached_len();if(nz&&n>=32&&off<=n-32&&n+36<=MAX_BLOCK){memmove(b+off+36,b+off,n-off);memcpy(b+off,token,32);*(u32*)(b+off+32)=0;cvm_cached_set_len(n+36);}cont();}
