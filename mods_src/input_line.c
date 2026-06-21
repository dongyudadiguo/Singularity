#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
__declspec(dllexport) void run(void) { char b[4096]; H o; cvm_zero(o); if(fgets(b,sizeof(b),stdin)){u32 n=(u32)strlen(b);while(n&&(b[n-1]=='\n'||b[n-1]=='\r'))n--;block_write((u8*)b,n,o);} cvm_push(o); cnext(); }
