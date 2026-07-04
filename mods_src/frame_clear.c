typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
#include "../dxgfx.h"
__declspec(dllexport) void run(void) { u32 c = 0xff000000u; if (cvm_payload_size() >= 4) c = *(u32*)cvm_payload(); dxgfx_clear(c); cont(); }
