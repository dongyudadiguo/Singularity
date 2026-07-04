typedef unsigned char u8; typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
__declspec(dllexport) void run(void){ float x=0,y=0,z=1; u8*p=cvm_payload(); if(cvm_payload_size()>=12){x=*(float*)p;y=*(float*)(p+4);z=*(float*)(p+8);} dxgfx_set_camera(x,y,z); cont(); }
