#include "mod.h"

__declspec(dllexport) void run(void) {
    cvm_scope_start();
    cont();
}