#include <string.h>
typedef unsigned u32; extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void *slot(u32 size);
#include "../dxgfx.h"
__declspec(dllexport) void run(void) { unsigned char kd[256], kp[256], kr[256]; int m[8]; char t[64]; dxgfx_input_snapshot(kd,kp,kr,m,t); memcpy(slot(256), kd, 256); memcpy(slot(256), kp, 256); memcpy(slot(256), kr, 256); memcpy(slot(sizeof(m)), m, sizeof(m)); memcpy(slot(64), t, 64); cont(); }
