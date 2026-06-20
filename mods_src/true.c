#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) {
    H t;
    cvm_zero(t);
    t[0] = 1;
    cvm_push(t);
    cnext();
}
