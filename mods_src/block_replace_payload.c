#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void *pop(u32 size); extern __declspec(dllimport) u8 *cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_cached_base(void); extern __declspec(dllimport) u32 cvm_cached_len(void); extern __declspec(dllimport) void cvm_cached_set_len(u32 n);
#define MAX_BLOCK (1u<<20)
__declspec(dllexport) void run(void) { u32 off=*(u32*)pop(4); u8 *np=cvm_payload(); u32 nn=cvm_payload_size(); u8 *b=cvm_cached_base(); u32 l=cvm_cached_len(); if(off+36>l){cont();return;} u32 old=*(u32*)(b+off+32); if(off+36+old>l || l-old+nn>MAX_BLOCK){cont();return;} memmove(b+off+36+nn,b+off+36+old,l-(off+36+old)); *(u32*)(b+off+32)=nn; memcpy(b+off+36,np,nn); cvm_cached_set_len(l-old+nn); cont(); }
