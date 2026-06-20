#include "net_ops.h"
static H BOOT_KEY_SB = {0x43,0x56,0x4d,0x5f,0x42,0x4f,0x4f,0x54};
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s) net_uset(BOOT_KEY_SB,s->view_hash); cnext(); }
