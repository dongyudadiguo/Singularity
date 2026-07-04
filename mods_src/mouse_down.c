typedef unsigned char u8; typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
__declspec(dllexport) void run(void){ int m[4]={0}; u32 mask=1,r; if(cvm_payload_size()>=4)mask=*(u32*)cvm_payload(); dxgfx_mouse(m); r=(m[2]&mask)?1:0; push(&r,4); cont(); }
