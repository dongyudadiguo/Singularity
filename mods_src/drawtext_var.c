typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void); extern __declspec(dllimport) u8*cvm_var_get(const H,u32*);
#include "../dxgfx.h"
/* payload: id[32], s32 x, s32 y, u32 ARGB, f32 size */
__declspec(dllexport) void run(void){u8*p=cvm_payload();if(cvm_payload_size()>=48){u32 n;u8*s=cvm_var_get(p,&n);if(s){u32 z=0;while(z<n&&s[z])z++;dxgfx_draw_text(*(int*)(p+32),*(int*)(p+36),*(u32*)(p+40),*(float*)(p+44),(char*)s,z);}}cont();}
