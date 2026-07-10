#include <string.h>
typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32);
#include "../dxgfx.h"
__declspec(dllexport) void run(void){unsigned char kd[256],kp[256],kr[256];int m[8];char raw[64],out[256]={0};dxgfx_input_snapshot(kd,kp,kr,m,raw);u32 z=0;for(u32 i=0;i<63&&raw[i];i++)if((unsigned char)raw[i]>32)out[z++]=raw[i];push(out,sizeof(out));cont();}
