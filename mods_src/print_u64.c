#include "io_parse.h"
__declspec(dllexport) void run(void) { H h; cvm_pop(h); printf("%llu", cvm_h_to_u64(h)); cnext(); }
