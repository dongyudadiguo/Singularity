#include "net_ops.h"
static H BOOT_KEY_LB = {0x43,0x56,0x4d,0x5f,0x42,0x4f,0x4f,0x54};
__declspec(dllexport) void run(void) { H out; cvm_zero(out); if(!net_uget(BOOT_KEY_LB,out)) cvm_zero(out); CvmState*s=cvm_state(); if(s)memcpy(s->view_hash,out,32); cvm_push(out); cnext(); }
