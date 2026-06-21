#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H r,g,b,o; cvm_pop(b); cvm_pop(g); cvm_pop(r); u64 rv=cvm_h_to_u64(r)&255,gv=cvm_h_to_u64(g)&255,bv=cvm_h_to_u64(b)&255; cvm_u64_to_h(rv|(gv<<8)|(bv<<16),o); cvm_push(o); cnext(); }
