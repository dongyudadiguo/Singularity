#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H ms; cvm_pop(ms); Sleep((DWORD)cvm_h_to_u64(ms)); cnext(); }
