#include <string.h>
typedef unsigned char u8; typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void); extern __declspec(dllimport) u8*cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void){ u8*p=cvm_payload(); if(cvm_payload_size()<8){cont();return;} u32 off=*(u32*)p,n=*(u32*)(p+4),len=cvm_cached_len(); if(off+n<=len) push(cvm_cached_base()+off,n); cont(); }
