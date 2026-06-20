#include "block_chain.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s){u32 len=0,st=0,en=0;u8*d=block_read(s->view_hash,&len);H out;cvm_zero(out);if(d){if(bc_range(d,len,(u32)s->view_index,&st,&en)){bc_save_edit(d,st,d+en,len-en,0,0,out);memcpy(s->view_hash,out,32);}free(d);}} cnext(); }
