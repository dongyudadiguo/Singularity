#include "mod.h"

__declspec(dllexport) void run(void) {
    cvm_reexec();
}
