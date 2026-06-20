#include "../cvm_state.h"
#include "../continue.h"
static H BOOT_KEY = {0x43,0x56,0x4d,0x5f,0x42,0x4f,0x4f,0x54};
__declspec(dllexport) void run(void) { cvm_push(BOOT_KEY); cnext(); }
