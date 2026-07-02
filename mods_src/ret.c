#include "mod.h"

__declspec(dllexport) void run(void) {
    if (cvm_ret()) cont();
}
