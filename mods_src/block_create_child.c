#include <string.h>
typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void *p, u32 size); extern __declspec(dllimport) u8 *cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) int cvm_sha256(const u8 *p, u32 n, H out); extern __declspec(dllimport) void cvm_edge(const H parent, const H child);
__declspec(dllexport) void run(void) { H k; u8 z[32]={0}; u8 name[96]; u32 n=cvm_payload_size(); if(n>sizeof(name)) n=sizeof(name); memcpy(name,cvm_payload(),n); cvm_sha256(name,n,k); cvm_edge(k,z); push(k,32); cont(); }
