#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void *pop(u32 size); extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void);
__declspec(dllexport) void run(void) { u32 off=*(u32*)pop(4); u8 *b=cvm_cached_base(); u32 l=cvm_cached_len(); if(off+36<=l){u32 n=*(u32*)(b+off+32); if(off+36+n<=l) push(b+off+36,n);} cont(); }
