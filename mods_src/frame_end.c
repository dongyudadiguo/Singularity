extern __declspec(dllimport) void cont(void);
#include "../dxgfx.h"
__declspec(dllexport) void run(void) { dxgfx_frame_end(); cont(); }
