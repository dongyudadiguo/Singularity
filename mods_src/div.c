#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H a,b,o; cvm_pop(b); cvm_pop(a); u64 d=cvm_h_to_u64(b); cvm_u64_to_h(d?cvm_h_to_u64(a)/d:0, o); cvm_push(o);     cnext();
}
