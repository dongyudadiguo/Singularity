// Win11 + MinGW-w64 x86/x64 only.
// gcc cvm.c -Os -s -o cvm.exe -lws2_32
// VM 无预设指令；mods/*.dll 动态注册。
// TCP binary RPC: req = op:u8 + len:u32be + body.
// Op void；VM 不替指令 adv。

#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CVM_SERVER
#define CVM_SERVER "118.25.42.70"
#endif

#ifndef CVM_PORT
#define CVM_PORT 9000
#endif

#define H 32
#define N 256

// RPC opcodes
#define OP_REGISTER  1
#define OP_FILE      2
#define OP_CHILDREN  3
#define OP_EDGE      4
#define OP_VOTE      5
#define OP_UPLOAD    6
#define OP_USET      7
#define OP_UGET      8

typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;
typedef void (*Op)(u8 *, uint32_t);

typedef struct { u8 off; Buf f; u8 key[H]; } Frame;

typedef struct {
    void (*op)(u8 *, Op);
    void (*op_name)(char *, Op);
    void (*del)(u8 *);
    void (*del_name)(char *);
    void (*override)(u8 *, u8 *, DWORD);
    void (*touch)(void);
    Buf (*rpc)(uint8_t, u8 *, DWORD);
    void (*run)(u8 *);
    void (*enter)(u8 *);
    void (*adv)(void);
    void (*push)(u8 *, uint32_t);
    Buf (*pop)(void);
    Buf *(*top)(void);
    Frame *cur;
} Host;

typedef struct { u8 id[H]; Op fn; } Ins;
typedef struct { u8 key[H]; Buf f; } Ov;

static SOCKET sock;
static u8 ZERO[H];
static Frame cur, ret[N];
static int active, rn;
static Ins ins[N]; static int insn;
static Ov ov[N]; static int ovn;
static Buf st[N]; static int sn;
void (*imp)();

static void root(); static void step();

static uint32_t U(u8 *p) { uint32_t x; memcpy(&x, p, 4); return x; }
static int Z(u8 *p) { for (int i = 0; i < H; i++) if (p[i]) return 0; return 1; }
static Buf B(u8 *p, DWORD n) { Buf b = { malloc(n), n }; if (n) memcpy(b.p, p, n); return b; }
static void T(char *s, u8 o[H]) { DWORD n = (DWORD)strlen(s); memset(o, 0, H); memcpy(o, s, n > H ? H : n); }
static DWORD BE(u8 *p) { return ((DWORD)p[0] << 24) | ((DWORD)p[1] << 16) | ((DWORD)p[2] << 8) | p[3]; }
static void WBE(u8 *p, DWORD n) { p[0] = n >> 24; p[1] = n >> 16; p[2] = n >> 8; p[3] = n; }

static void sendall(u8 *p, int n) { while (n) { int k = send(sock, (char *)p, n, 0); if (k <= 0) { sock = INVALID_SOCKET; return; } p += k; n -= k; } }
static void recvall(u8 *p, int n) { while (n) { int k = recv(sock, (char *)p, n, 0); if (k <= 0) { sock = INVALID_SOCKET; return; } p += k; n -= k; } }

Buf cvm_rpc(uint8_t op, u8 *body, DWORD len) {
    u8 h5[5]; Buf b = {0};
    h5[0] = op; WBE(h5 + 1, len);
    sendall(h5, 5); if (sock == INVALID_SOCKET) return b;
    if (len) { sendall(body, len); if (sock == INVALID_SOCKET) return b; }
    recvall(h5, 5); if (sock == INVALID_SOCKET) return b;
    b.n = BE(h5 + 1); b.p = malloc(b.n);
    if (b.n) { recvall(b.p, b.n); if (sock == INVALID_SOCKET) { free(b.p); b.p = 0; b.n = 0; } }
    return b;
}

void cvm_push(u8 *p, uint32_t n) { st[sn++] = B(p, n); }
Buf cvm_pop() { Buf z = {0}; return sn ? st[--sn] : z; }
Buf *cvm_top() { return sn ? st + sn - 1 : 0; }
void cvm_op(u8 *id, Op fn) { memcpy(ins[insn].id, id, H); ins[insn++].fn = fn; }
void cvm_op_name(char *name, Op fn) { u8 id[H]; T(name, id); cvm_op(id, fn); }
void cvm_del(u8 *id) { memcpy(ins[insn].id, id, H); ins[insn++].fn = 0; }
void cvm_del_name(char *name) { u8 id[H]; T(name, id); cvm_del(id); }

static Op opfind(u8 *id) { for (int i = insn - 1; i >= 0; i--) if (!memcmp(ins[i].id, id, H)) return ins[i].fn; return 0; }
static Ov *ovfind(u8 *key) { for (int i = ovn - 1; i >= 0; i--) if (!memcmp(ov[i].key, key, H)) return ov + i; return 0; }

void cvm_override(u8 *key, u8 *file, DWORD len) {
    Ov *o = ovfind(key);
    if (!o) { o = ov + ovn++; memcpy(o->key, key, H); }
    free(o->f.p); o->f = B(file, len);
}
void cvm_touch() { cvm_override(cur.key, cur.f.p, cur.f.n); }

static void firstchild(u8 *parent, u8 child[H]) {
    Buf r = cvm_rpc(OP_CHILDREN, parent, H);
    if (r.p && r.n >= 36) memcpy(child, r.p + 4, H); else memset(child, 0, H);
    free(r.p);
}
static Buf file(u8 *hash) { return cvm_rpc(OP_FILE, hash, H); }
static Buf keyfile(u8 *key) {
    Ov *o = ovfind(key); u8 h[H];
    if (o) return B(o->f.p, o->f.n);
    firstchild(key, h); return file(h);
}

void cvm_adv() { cur.off += H + U(cur.f.p + cur.off + H); }
void cvm_enter(u8 *key) {
    if (active) { cvm_adv(); ret[rn++] = cur; }
    cur.f = keyfile(key); cur.off = 0; memcpy(cur.key, key, H); active = 1;
}
static void leave() { free(cur.f.p); if (rn) cur = ret[--rn]; else active = 0; }

void cvm_run(u8 *h) {
    Op op = opfind(h);
    if (op) { u8 *p = cur.f.p + cur.off; op(p + H + 4, U(p + H) - 4); }
    else cvm_enter(h);
}

static void root() { cvm_run(ZERO); imp = step; }
static void step() {
    if (!active) { imp = root; return; }
    if (cur.off + H > cur.f.n || Z(cur.f.p + cur.off)) { leave(); return; }
    cvm_run(cur.f.p + cur.off);
}

static Host host = {
    cvm_op, cvm_op_name, cvm_del, cvm_del_name,
    cvm_override, cvm_touch, cvm_rpc,
    cvm_run, cvm_enter, cvm_adv,
    cvm_push, cvm_pop, cvm_top, &cur
};

typedef void (*ModInit)(Host *);
static void loadmod(char *p) { ((ModInit)GetProcAddress(LoadLibraryA(p), "cvm_init"))(&host); }
static int cmpmod(const void *a, const void *b) { return lstrcmpiA((char *)a, (char *)b); }

static void loadmods() {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("mods\\*.dll", &fd);
    char ms[N][MAX_PATH]; int n = 0;
    if (h == INVALID_HANDLE_VALUE) return;
    do { wsprintfA(ms[n++], "mods\\%s", fd.cFileName); } while (n < N && FindNextFileA(h, &fd));
    FindClose(h);
    qsort(ms, n, sizeof ms[0], cmpmod);
    for (int i = 0; i < n; i++) loadmod(ms[i]);
}

void boot() {
    WSADATA w; struct sockaddr_in a;
    WSAStartup(MAKEWORD(2, 2), &w);
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) { sock = 0; return; }
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_port = htons(CVM_PORT); a.sin_addr.s_addr = inet_addr(CVM_SERVER);
    { int to = 5000; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&to, sizeof to); setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&to, sizeof to); }
    if (connect(sock, (struct sockaddr *)&a, sizeof a) == SOCKET_ERROR) { closesocket(sock); sock = INVALID_SOCKET; return; }
    loadmods(); imp = root;
}

int main() {
    boot();
    while (1) {
        MSG msg;
        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        if (imp) imp(); else Sleep(16);
    }
}