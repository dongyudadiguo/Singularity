#include <string.h>
typedef unsigned char u8; typedef unsigned u32; typedef u8 H[32];
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void); extern __declspec(dllimport) int cvm_sha256(const u8*,u32,H);
#include "../uistate.h"
__declspec(dllexport) void run(void){ UiState*s=ui_state(); if(!s->ready){ ui_reset(); s=ui_state(); if(cvm_payload_size()>=32)memcpy(s->program_key,cvm_payload(),32); else cvm_sha256((u8*)"#SingularityProgram",19,s->program_key); cvm_sha256((u8*)"#TAG",4,s->registry_key); s->views[0].used=1; memcpy(s->views[0].key,s->program_key,32); s->views[0].x=40; s->views[0].y=70; s->view_count=1; s->ready=1; } cont(); }
