typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void *from(u32); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
/* payload: s32 x, s32 y, u32 ARGB, f32 size, u32 byte_count */
__declspec(dllexport) void run(void){u8*p=cvm_payload();if(cvm_payload_size()>=20){u32 n=*(u32*)(p+16);char*s=(char*)from(n);dxgfx_draw_text(*(int*)p,*(int*)(p+4),*(u32*)(p+8),*(float*)(p+12),s,n);}cont();}
