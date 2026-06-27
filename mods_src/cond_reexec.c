#include "mod.h"

__declspec(dllexport) void run(void) {
    if (mod_bool(pop())) cvm_reexec();
    else cont();
}
