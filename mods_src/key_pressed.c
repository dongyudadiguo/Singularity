typedef unsigned char u8; typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
__declspec(dllexport) void run(void){ u32 vk=0,r=0; if(cvm_payload_size()>=4)vk=*(u32*)cvm_payload(); r=dxgfx_key_state((int)vk,1); push(&r,4); cont(); }
