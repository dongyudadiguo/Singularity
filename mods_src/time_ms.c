#include "../cvm_state.h"
#include "../continue.h"

__declspec(dllexport) void run(void) {
    H out;
    cvm_u64_to_h(GetTickCount64(), out);
    cvm_push(out);
    cnext();
}
