#include <string.h>
typedef unsigned char u8; typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void); extern __declspec(dllimport) u8*cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void){ u8*p=cvm_payload(); if(cvm_payload_size()<12){cont();return;} u32 src=*(u32*)p,n=*(u32*)(p+4),dst=*(u32*)(p+8),len=cvm_cached_len(); u8*b=cvm_cached_base(); if(src+n<=len&&dst+n<=len) memmove(b+dst,b+src,n); cont(); }
