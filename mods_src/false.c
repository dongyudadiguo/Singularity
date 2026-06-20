#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    H f;
    cvm_zero(f);
    cvm_push(f);
    cnext();
}
