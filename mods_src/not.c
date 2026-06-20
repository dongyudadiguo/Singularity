#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H a,o; cvm_pop(a); cvm_u64_to_h(!cvm_truth(a), o); cvm_push(o);     cnext();
}
