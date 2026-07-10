typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
/* payload: f32 target_x, f32 target_y, f32 zoom */
__declspec(dllexport) void run(void){u8*p=cvm_payload();if(cvm_payload_size()>=12)dxgfx_set_camera(*(float*)p,*(float*)(p+4),*(float*)(p+8));cont();}
