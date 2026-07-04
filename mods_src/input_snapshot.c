typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void *p, u32 size);
#include "../dxgfx.h"
__declspec(dllexport) void run(void) { unsigned char kd[256], kp[256], kr[256]; int m[8]; char t[64]; dxgfx_input_snapshot(kd,kp,kr,m,t); push(kd,256); push(kp,256); push(kr,256); push(m,sizeof(m)); push(t,64); cont(); }
