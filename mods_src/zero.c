#include "../cvm_state.h"
__declspec(dllexport) void run(void) {
    H z;
    cvm_zero(z);
    cvm_push(z);
}
