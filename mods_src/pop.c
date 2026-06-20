#include "../cvm_state.h"
#include "../continue.h"
__declspec(dllexport) void run(void) { H v; cvm_pop(v);     cnext();
}
