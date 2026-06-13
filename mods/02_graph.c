#include "cvm_mod.h"

static Host *G;

static void arg32(u8 *d, uint32_t n, u8 out[32]) {
    if (n >= 32) {
        memcpy(out, d, 32);
    } else {
        Buf a = G->pop();
        memcpy(out, a.p, 32);
        free(a.p);
    }
}

static int g_file(u8 *d, uint32_t n) {
    u8 h[32];
    arg32(d, n, h);
    Buf r = G->post(L"/api/file", h, 32);
    G->push(r.p, r.n);
    free(r.p);
    return 0;
}

static int g_child0(u8 *d, uint32_t n) {
    u8 h[32];
    arg32(d, n, h);
    Buf r = G->post(L"/api/children", h, 32);
    G->push(r.p + 4, 32);
    free(r.p);
    return 0;
}

static int g_childs(u8 *d, uint32_t n) {
    u8 h[32];
    arg32(d, n, h);
    Buf r = G->post(L"/api/children", h, 32);
    G->push(r.p, r.n);
    free(r.p);
    return 0;
}

static int g_upload(u8 *d, uint32_t n) {
    Buf a;
    if (n) a.p = d, a.n = n;
    else a = G->pop();

    Buf r = G->post(L"/api/upload", a.p, a.n);
    G->push(r.p, r.n);

    if (!n) free(a.p);
    free(r.p);
    return 0;
}

static int g_edge(u8 *d, uint32_t n) {
    u8 body[64];

    if (n >= 64) memcpy(body, d, 64);
    else {
        Buf child = G->pop(), parent = G->pop();
        memcpy(body, parent.p, 32);
        memcpy(body + 32, child.p, 32);
        free(parent.p); free(child.p);
    }

    Buf r = G->post(L"/api/edge", body, 64);
    free(r.p);
    return 0;
}

static int g_vote(u8 *d, uint32_t n) {
    u8 body[96];

    if (n >= 96) memcpy(body, d, 96);
    else {
        Buf child = G->pop(), parent = G->pop(), user = G->pop();
        memcpy(body, user.p, 32);
        memcpy(body + 32, parent.p, 32);
        memcpy(body + 64, child.p, 32);
        free(user.p); free(parent.p); free(child.p);
    }

    Buf r = G->post(L"/api/vote", body, 96);
    free(r.p);
    return 0;
}

static int g_uget(u8 *d, uint32_t n) {
    u8 body[64];

    if (n >= 64) memcpy(body, d, 64);
    else {
        Buf key = G->pop(), user = G->pop();
        memcpy(body, user.p, 32);
        memcpy(body + 32, key.p, 32);
        free(user.p); free(key.p);
    }

    Buf r = G->post(L"/api/user/get", body, 64);
    G->push(r.p, r.n);
    free(r.p);
    return 0;
}

static int g_uset(u8 *d, uint32_t n) {
    u8 body[96];

    if (n >= 96) memcpy(body, d, 96);
    else {
        Buf val = G->pop(), key = G->pop(), user = G->pop();
        memcpy(body, user.p, 32);
        memcpy(body + 32, key.p, 32);
        memcpy(body + 64, val.p, 32);
        free(user.p); free(key.p); free(val.p);
    }

    Buf r = G->post(L"/api/user/set", body, 96);
    free(r.p);
    return 0;
}

static int g_register(u8 *d, uint32_t n) {
    Buf a;
    if (n) a.p = d, a.n = n;
    else a = G->pop();

    Buf r = G->post(L"/api/register", a.p, a.n);
    G->push(r.p, r.n);

    if (!n) free(a.p);
    free(r.p);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:G:FILE", g_file);
    h->op_name("CVM1:G:CHILD0", g_child0);
    h->op_name("CVM1:G:CHILDS", g_childs);
    h->op_name("CVM1:G:UPLOAD", g_upload);
    h->op_name("CVM1:G:EDGE", g_edge);
    h->op_name("CVM1:G:VOTE", g_vote);
    h->op_name("CVM1:G:UGET", g_uget);
    h->op_name("CVM1:G:USET", g_uset);
    h->op_name("CVM1:G:REGISTER", g_register);
}
