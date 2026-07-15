#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib,"ws2_32.lib")

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef void (*Fn)();

__declspec(dllexport) SOCKET conn;
__declspec(dllexport) Fn imp;

void rd(void *b, u32 n) {
    for (u8 *p = b; n;) {
        int r = recv(conn, (char *)p, n, 0);
        if (r <= 0) return;
        p += r;
        n -= (u32)r;
    }
}
void op(u8 o, void *b, u32 n) {
    u8 h[5] = { o, (u8)(n >> 24), (u8)(n >> 16), (u8)(n >> 8), (u8)n };
    send(conn, (char *)h, 5, 0);
    if (n) send(conn, b, n, 0);
}
u8 *rx(void) {
    u8 h[5];
    rd(h, 5);
    u32 n = (u32)h[1] << 24 | (u32)h[2] << 16 | (u32)h[3] << 8 | h[4];
    u8 *b = (u8 *)malloc(n ? n : 1);
    if (n) rd(b, n);
    return b;
}

/* Children listing (still used by bootstrap / graph). No content download in main. */
__declspec(dllexport) void cvm_firstchild(H p, H c) {
    op(5, p, 32);
    u8 *b = rx();
    if (b) {
        memcpy(c, b + 4, 32);
        free(b);
    } else memset(c, 0, 32);
}

#define FIND_CACHE 256
typedef struct { H key; Fn fn; int on; } FindSlot;
static FindSlot find_slots[FIND_CACHE];

static u32 find_hash(const H h) {
    return (u32)h[0] | ((u32)h[1] << 8) | ((u32)h[2] << 16) | ((u32)h[3] << 24);
}

__declspec(dllexport) Fn find(H h) {
    u32 idx = find_hash(h) & (FIND_CACHE - 1);
    for (u32 n = 0; n < FIND_CACHE; n++) {
        u32 i = (idx + n) & (FIND_CACHE - 1);
        if (find_slots[i].on && !memcmp(find_slots[i].key, h, 32))
            return find_slots[i].fn;
        if (!find_slots[i].on) {
            char p[75] = "mods/";
            for (int k = 0; k < 32; k++) sprintf(p + 5 + k * 2, "%02x", h[k]);
            strcat(p, ".dll");
            HMODULE mod = LoadLibraryA(p);
            Fn f = mod ? (Fn)GetProcAddress(mod, "run") : 0;
            memcpy(find_slots[i].key, h, 32);
            find_slots[i].fn = f;
            find_slots[i].on = 1;
            return f;
        }
    }
    {
        char p[75] = "mods/";
        for (int k = 0; k < 32; k++) sprintf(p + 5 + k * 2, "%02x", h[k]);
        strcat(p, ".dll");
        HMODULE mod = LoadLibraryA(p);
        return mod ? (Fn)GetProcAddress(mod, "run") : 0;
    }
}

__declspec(dllexport) int cvm_has_dll(H h) {
    return find(h) != 0;
}

/*
 * Entry (data->data): do NOT download a content hash to get a DLL token.
 * Load local first_bootstrap_block.bin:
 *   token_len=32 + bootstrap_token[32] + payload_len=0 + end
 * Then find(bootstrap_token) and run. bootstrap.dll firstchild(self)->first block
 * and cvm_exec resolves override/content via vmstore (no hash middle layer in main).
 */
static int load_bootstrap_token(H out) {
    FILE *f = fopen("first_bootstrap_block.bin", "rb");
    u8 buf[64];
    u32 tlen, plen;
    if (!f) return 0;
    if (fread(buf, 1, 8 + 32, f) < 8 + 32) { fclose(f); return 0; }
    fclose(f);
    tlen = *(u32 *)buf;
    if (tlen != 32) return 0;
    memcpy(out, buf + 4, 32);
    plen = *(u32 *)(buf + 4 + 32);
    (void)plen;
    return 1;
}

int main(void) {
    WSADATA w;
    H boot = {0};
    struct sockaddr_in a = {0};
    WSAStartup(0x202, &w);
    conn = socket(2, 1, 0);
    a.sin_family = 2;
    a.sin_port = htons(9000);
    inet_pton(2, "127.0.0.1", &a.sin_addr);
    connect(conn, (void *)&a, sizeof a);

    if (!load_bootstrap_token(boot)) {
        /* fallback: graph root children[0] is bootstrap token (edge only, not file body) */
        cvm_firstchild(boot, boot);
    }
    imp = find(boot);
    if (!imp) return 1;
    for (;;) imp();
}
