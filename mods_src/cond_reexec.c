#include "mod.h"

__declspec(dllexport) void run(void) {
    if (mod_bool(pop(4))) cvm_reexec();
    else cont();
}
