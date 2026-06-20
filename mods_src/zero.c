#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    H z;
    cvm_zero(z);
    cvm_push(z);
    cnext();
}
