// Win11 + MinGW-w64. gcc cvm.c -Os -s -o cvm.exe -lws2_32
// VM 无预设指令；mods/*.dll 注册。TCP RPC: op:u8 + len:u32be + body.
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
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
#define OP_FILE 2
#define OP_CHILDREN 3
typedef unsigned char u8;
typedef struct
{
    u8 *p;
    uint32_t n;
} Buf;
typedef void (*Op)(u8 *, uint32_t);
typedef struct
{
    u8 off;
    Buf f;
    u8 key[H];
} Frame;
typedef struct
{
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
typedef struct
{
    u8 id[H];
    Op fn;
} Ins;
typedef struct
{
    u8 key[H];
    Buf f;
} Ov;

static SOCKET sock;
static u8 ZERO[H];
static Frame cur, ret[N];
static int active, rn, insn, ovn, sn;
static Ins ins[N];
static Ov ov[N];
static Buf st[N];
void (*imp)(void);
static void root(void), step(void);

static uint32_t U(u8 *p)
{
    uint32_t x;
    memcpy(&x, p, 4);
    return x;
}
static int Z(u8 *p)
{
    for (int i = 0; i < H; i++)
        if (p[i])
            return 0;
    return 1;
}
static Buf B(u8 *p, DWORD n)
{
    Buf b = {malloc(n), n};
    if (n)
        memcpy(b.p, p, n);
    return b;
}
static void T(char *s, u8 o[H])
{
    DWORD n = strlen(s);
    memset(o, 0, H);
    memcpy(o, s, n > H ? H : n);
}

// 收发合一；大小端用 winsock 自带 htonl/ntohl
static int xfer(u8 *p, int n, int rd)
{
    while (n)
    {
        int k = rd ? recv(sock, (char *)p, n, 0) : send(sock, (char *)p, n, 0);
        if (k <= 0)
        {
            sock = INVALID_SOCKET;
            return 0;
        }
        p += k;
        n -= k;
    }
    return 1;
}

Buf cvm_rpc(uint8_t op, u8 *body, DWORD len)
{
    u8 h[5];
    Buf b = {0};
    uint32_t be = htonl(len);
    h[0] = op;
    memcpy(h + 1, &be, 4);
    if (!xfer(h, 5, 0) || (len && !xfer(body, len, 0)) || !xfer(h, 5, 1))
        return b;
    b.n = ntohl(*(uint32_t *)(h + 1));
    b.p = malloc(b.n);
    if (b.n && !xfer(b.p, b.n, 1))
    {
        free(b.p);
        b.p = 0;
        b.n = 0;
    }
    return b;
}

void cvm_push(u8 *p, uint32_t n) { st[sn++] = B(p, n); }
Buf cvm_pop(void)
{
    Buf z = {0};
    return sn ? st[--sn] : z;
}
Buf *cvm_top(void) { return sn ? st + sn - 1 : 0; }

void cvm_op(u8 *id, Op fn)
{
    memcpy(ins[insn].id, id, H);
    ins[insn++].fn = fn;
}
void cvm_op_name(char *s, Op fn)
{
    u8 id[H];
    T(s, id);
    cvm_op(id, fn);
}
void cvm_del(u8 *id)
{
    memcpy(ins[insn].id, id, H);
    ins[insn++].fn = 0;
}
void cvm_del_name(char *s)
{
    u8 id[H];
    T(s, id);
    cvm_del(id);
}
static Op opfind(u8 *id)
{
    for (int i = insn - 1; i >= 0; i--)
        if (!memcmp(ins[i].id, id, H))
            return ins[i].fn;
    return 0;
}

static Ov *ovfind(u8 *k)
{
    for (int i = ovn - 1; i >= 0; i--)
        if (!memcmp(ov[i].key, k, H))
            return ov + i;
    return 0;
}
void cvm_override(u8 *k, u8 *f, DWORD n)
{
    Ov *o = ovfind(k);
    if (!o)
    {
        o = ov + ovn++;
        memcpy(o->key, k, H);
    }
    free(o->f.p);
    o->f = B(f, n);
}
void cvm_touch(void) { cvm_override(cur.key, cur.f.p, cur.f.n); }

static Buf keyfile(u8 *k)
{
    Ov *o = ovfind(k);
    u8 h[H] = {0};
    if (o)
        return B(o->f.p, o->f.n);
    Buf r = cvm_rpc(OP_CHILDREN, k, H);
    if (r.p && r.n >= 36)
        memcpy(h, r.p + 4, H);
    free(r.p);
    return cvm_rpc(OP_FILE, h, H);
}

void cvm_adv(void) { cur.off += H + U(cur.f.p + cur.off + H); }
void cvm_enter(u8 *k)
{
    if (active)
    {
        cvm_adv();
        ret[rn++] = cur;
    }
    cur.f = keyfile(k);
    cur.off = 0;
    memcpy(cur.key, k, H);
    active = 1;
}
static void leave(void)
{
    free(cur.f.p);
    if (rn)
        cur = ret[--rn];
    else
        active = 0;
}
void cvm_run(u8 *h)
{
    Op op = opfind(h);
    if (op)
    {
        u8 *p = cur.f.p + cur.off;
        op(p + H + 4, U(p + H) - 4);
    }
    else
        cvm_enter(h);
}
static void root(void)
{
    cvm_run(ZERO);
    imp = step;
}
static void step(void)
{
    if (!active)
    {
        imp = root;
        return;
    }
    if (cur.off + H > cur.f.n || Z(cur.f.p + cur.off))
    {
        leave();
        return;
    }
    cvm_run(cur.f.p + cur.off);
}

static Host host = {cvm_op, cvm_op_name, cvm_del, cvm_del_name, cvm_override, cvm_touch, cvm_rpc,
                    cvm_run, cvm_enter, cvm_adv, cvm_push, cvm_pop, cvm_top, &cur};
typedef void (*ModInit)(Host *);
static int cmpmod(const void *a, const void *b) { return lstrcmpiA((char *)a, (char *)b); }
static void loadmods(void)
{
    WIN32_FIND_DATAA fd;
    char ms[N][MAX_PATH];
    int n = 0;
    HANDLE h = FindFirstFileA("mods\\*.dll", &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do
        wsprintfA(ms[n++], "mods\\%s", fd.cFileName);
    while (n < N && FindNextFileA(h, &fd));
    FindClose(h);
    qsort(ms, n, sizeof ms[0], cmpmod);
    for (int i = 0; i < n; i++)
        ((ModInit)GetProcAddress(LoadLibraryA(ms[i]), "cvm_init"))(&host);
}

void boot(void)
{
    WSADATA w;
    struct sockaddr_in a = {0};
    int to = 5000;
    imp = root;
    WSAStartup(MAKEWORD(2, 2), &w);
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        sock = 0;
        return;
    }
    a.sin_family = AF_INET;
    a.sin_port = htons(CVM_PORT);
    a.sin_addr.s_addr = inet_addr(CVM_SERVER);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&to, sizeof to);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&to, sizeof to);
    if (connect(sock, (struct sockaddr *)&a, sizeof a) == SOCKET_ERROR)
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
        return;
    }
    loadmods();
}

int main(void)
{
    boot();
    while (1)
        imp();
}