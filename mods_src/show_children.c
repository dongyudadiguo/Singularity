#include "net_ops.h"
#include "io_parse.h"
__declspec(dllexport) void run(void) { CvmState*s=cvm_state(); if(s){u8*d=0;u32 len=0;if(net_children(s->view_hash,&d,&len)){u32 n=len>=4?net_u32be(d):0;for(u32 i=0;i<n;i++){if(4+i*40+32<=len){printf("%u ",i);H h;memcpy(h,d+4+i*40,32);print_h(h);printf("\n");}}free(d);}} cnext(); }
