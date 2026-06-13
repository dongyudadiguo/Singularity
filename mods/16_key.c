#include "cvm_mod.h"

static Host *G;

static void key(int vk) {
    u8 o[4];
    wr32(o, (GetAsyncKeyState(vk) & 0x8000) != 0);
    G->push(o, 4);
}

#define K(fn, vk) static int fn(u8 *d, uint32_t n) { key(vk); return 0; }

K(k_esc, VK_ESCAPE)
K(k_enter, VK_RETURN)
K(k_back, VK_BACK)
K(k_del, VK_DELETE)
K(k_tab, VK_TAB)
K(k_space, VK_SPACE)
K(k_left, VK_LEFT)
K(k_right, VK_RIGHT)
K(k_up, VK_UP)
K(k_down, VK_DOWN)
K(k_home, VK_HOME)
K(k_end, VK_END)
K(k_pgup, VK_PRIOR)
K(k_pgdn, VK_NEXT)

static int k_ascii(u8 *d, uint32_t n) {
    int vk;

    if (n) vk = VkKeyScanA(d[0]) & 255;
    else {
        Buf a = G->pop();
        vk = VkKeyScanA(a.p[0]) & 255;
        free(a.p);
    }

    key(vk);
    return 0;
}

static int k_mods(u8 *d, uint32_t n) {
    u8 o[4];
    uint32_t m = 0;

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) m |= 1;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) m |= 2;
    if (GetAsyncKeyState(VK_MENU) & 0x8000) m |= 4;

    wr32(o, m);
    G->push(o, 4);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:KEY:ESC", k_esc);
    h->op_name("CVM1:KEY:ENTER", k_enter);
    h->op_name("CVM1:KEY:BACK", k_back);
    h->op_name("CVM1:KEY:DEL", k_del);
    h->op_name("CVM1:KEY:TAB", k_tab);
    h->op_name("CVM1:KEY:SPACE", k_space);

    h->op_name("CVM1:KEY:LEFT", k_left);
    h->op_name("CVM1:KEY:RIGHT", k_right);
    h->op_name("CVM1:KEY:UP", k_up);
    h->op_name("CVM1:KEY:DOWN", k_down);
    h->op_name("CVM1:KEY:HOME", k_home);
    h->op_name("CVM1:KEY:END", k_end);
    h->op_name("CVM1:KEY:PGUP", k_pgup);
    h->op_name("CVM1:KEY:PGDN", k_pgdn);

    h->op_name("CVM1:KEY:ASCII", k_ascii);
    h->op_name("CVM1:KEY:MODS", k_mods);
}
