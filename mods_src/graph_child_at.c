#include "net_ops.h"
__declspec(dllexport) void run(void) { H parent,idx_h,out; u8*d=0; u32 len=0; cvm_pop(idx_h); cvm_pop(parent); cvm_zero(out); if(net_children(parent,&d,&len)){u64 idx=cvm_h_to_u64(idx_h); u64 off=4+idx*40; if(off+32<=len)memcpy(out,d+off,32); free(d);} cvm_push(out); cnext(); }
