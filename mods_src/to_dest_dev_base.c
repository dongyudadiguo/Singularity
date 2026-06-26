#include "surface_ops.h"
#include "net_ops.h"

static int g_init = 0;
static H g_boot_key = {0x43, 0x56, 0x4d, 0x5f, 0x42, 0x4f, 0x4f, 0x54};
static H g_view_hash;
static u8 *g_chain;
static u32 g_chain_len;

__declspec(dllexport) void run(void) {
    CvmState *s = cvm_state();
    if (!s) { cnext(); return; }

    if (!g_init) {
        g_init = 1;

        // Load boot view from user storage
        cvm_zero(g_view_hash);
        if (!net_uget(g_boot_key, g_view_hash)) {
            // If no boot key set, push empty and continue
            cvm_push(g_view_hash);
            cnext();
            return;
        }

        // Store as current view
        memcpy(s->view_hash, g_view_hash, 32);
        memcpy(s->cur_hash, g_view_hash, 32);

        // Load the chain data
        u32 len = 0;
        u8 *p = block_read(g_view_hash, &len);
        if (!p || !len) {
            cvm_push(g_view_hash);
            cnext();
            return;
        }
        g_chain = p;
        g_chain_len = len;
    }

    // Poll surface events and continue chain execution
    MSG msg;
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // If we have a chain, execute it as a nested call
    if (g_chain && g_chain_len) {
        CvmCallFrame frame;
        cframe_save(s, &frame);

        jmp_buf jb;
        jmp_buf *old = s->ret_jb;
        s->ret_jb = &jb;

        if (setjmp(jb) == 0) {
            cbegin(g_chain, g_chain_len);
        }

        s->ret_jb = old;
        cframe_restore(s, &frame);
    }

    // Push view hash and continue
    cvm_push(g_view_hash);
    cnext();
}