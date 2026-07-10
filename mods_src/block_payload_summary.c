#include <stdio.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void*pop(u32); extern __declspec(dllimport) void push(const void*,u32);
extern __declspec(dllimport) u8*cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void){u32 o=*(u32*)pop(4);char out[96]={0};u8*b=cvm_cached_base();u32 l=cvm_cached_len();if(o+36<=l){u32 n=*(u32*)(b+o+32);if(n&&o+36+n<=l){u32 z=n<42?n:42;int text=1;for(u32 i=0;i<z;i++)if(b[o+36+i]<32||b[o+36+i]>126)text=0;if(text)snprintf(out,sizeof(out),"'%.*s'",(int)z,b+o+36);else snprintf(out,sizeof(out),"[%u bytes]",n);}}push(out,sizeof(out));cont();}
