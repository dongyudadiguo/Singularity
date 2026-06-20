#ifndef MOD_NET_OPS_H
#define MOD_NET_OPS_H

#include "../cvm_state.h"
#include "../block.h"

static SOCKET net_sock = INVALID_SOCKET;
static int net_ready;
static H net_user;
static int net_user_ready;

static int net_init(void) {
    if (net_ready) return net_sock != INVALID_SOCKET;
    net_ready = 1;
    WSADATA w;
    WSAStartup(MAKEWORD(2,2), &w);
    net_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (net_sock == INVALID_SOCKET) return 0;
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    if (connect(net_sock, (void*)&a, sizeof(a)) != 0) { closesocket(net_sock); net_sock = INVALID_SOCKET; return 0; }
    return 1;
}

static void net_load_user(H out) {
    if (!net_user_ready) {
        FILE *f = fopen("id.bin", "rb");
        if (f) { fread(net_user, 1, 32, f); fclose(f); }
        else cvm_zero(net_user);
        net_user_ready = 1;
    }
    memcpy(out, net_user, 32);
}

static int net_send(u8 op, void *body, u32 len) {
    if (!net_init()) return 0;
    u8 h[5] = {op, len>>24, len>>16, len>>8, len};
    if (send(net_sock, (char*)h, 5, 0) != 5) return 0;
    if (len && send(net_sock, (char*)body, len, 0) != (int)len) return 0;
    return 1;
}

static u8* net_recv(u8 *status, u32 *out_len) {
    u8 h[5];
    u32 g = 0;
    while (g < 5) { int r = recv(net_sock, (char*)h + g, 5 - g, 0); if (r < 1) return 0; g += r; }
    if (status) *status = h[0];
    u32 len = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4];
    if (out_len) *out_len = len;
    if (!len) return 0;
    u8 *b = malloc(len + 1);
    if (!b) return 0;
    g = 0;
    while (g < len) { int r = recv(net_sock, (char*)b + g, len - g, 0); if (r < 1) { free(b); return 0; } g += r; }
    b[len] = 0;
    return b;
}

static int net_file(H h, u8 **data, u32 *len) {
    u8 st = 1;
    if (!net_send(3, h, 32)) return 0;
    u8 *b = net_recv(&st, len);
    if (st != 0) { if (b) free(b); return 0; }
    *data = b;
    return b != 0;
}

static int net_upload(u8 *data, u32 len, H out) {
    u8 st = 1;
    u32 rlen = 0;
    if (!net_send(2, data, len)) return 0;
    u8 *b = net_recv(&st, &rlen);
    if (st == 0 && b && rlen == 32) { memcpy(out, b, 32); free(b); return 1; }
    if (b) free(b);
    return 0;
}

static int net_edge(H parent, H child) {
    u8 body[64];
    u8 st = 1;
    u32 len = 0;
    memcpy(body, parent, 32); memcpy(body + 32, child, 32);
    if (!net_send(4, body, 64)) return 0;
    u8 *b = net_recv(&st, &len);
    if (b) free(b);
    return st == 0;
}

static int net_children(H parent, u8 **data, u32 *len) {
    u8 st = 1;
    if (!net_send(5, parent, 32)) return 0;
    u8 *b = net_recv(&st, len);
    if (st != 0) { if (b) free(b); return 0; }
    *data = b;
    return b != 0;
}

static int net_uget(H key, H out) {
    H user;
    u8 body[64];
    u8 st = 1;
    u32 len = 0;
    net_load_user(user);
    memcpy(body, user, 32); memcpy(body + 32, key, 32);
    if (!net_send(8, body, 64)) return 0;
    u8 *b = net_recv(&st, &len);
    if (st == 0 && b && len == 32) { memcpy(out, b, 32); free(b); return 1; }
    if (b) free(b);
    return 0;
}

static int net_uset(H key, H val) {
    H user;
    u8 body[96];
    u8 st = 1;
    u32 len = 0;
    net_load_user(user);
    memcpy(body, user, 32); memcpy(body + 32, key, 32); memcpy(body + 64, val, 32);
    if (!net_send(7, body, 96)) return 0;
    u8 *b = net_recv(&st, &len);
    if (b) free(b);
    return st == 0;
}

static int net_vote(H parent, H child) {
    H user;
    u8 body[96];
    u8 st = 1;
    u32 len = 0;
    net_load_user(user);
    memcpy(body, user, 32); memcpy(body + 32, parent, 32); memcpy(body + 64, child, 32);
    if (!net_send(6, body, 96)) return 0;
    u8 *b = net_recv(&st, &len);
    if (b) free(b);
    return st == 0;
}

static u32 net_u32be(u8 *p) { return (u32)p[0]<<24 | (u32)p[1]<<16 | (u32)p[2]<<8 | p[3]; }

#endif
