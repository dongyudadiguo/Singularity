// Win11 + MinGW-w64 x86/x64 only.
// gcc cvm.c -Os -s -o cvm.exe -lwinhttp
// VM 本体无预设指令；mods/*.dll 动态注册。
// Op 无返回值；VM 不替指令 adv；普通指令自己 h->adv()。

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef CVM_SERVER
#define CVM_SERVER L"124.221.146.23"
#endif

#ifndef CVM_PORT
#define CVM_PORT 9000
#endif

#define H 32
#define N 256

typedef unsigned char u8;

typedef struct
{
    u8 *p;
    DWORD n;
} Buf;
typedef struct
{
    Buf f;
    DWORD off;
    u8 key[H];
} Frame;
typedef void (*Op)(u8 *data, uint32_t len);

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

typedef struct Host
{
    void (*op)(u8 *id, Op fn);
    void (*op_name)(char *name, Op fn);
    void (*del)(u8 *id);
    void (*del_name)(char *name);

    void (*override)(u8 *key, u8 *file, DWORD len);
    void (*touch)();

    Buf (*post)(wchar_t *path, u8 *body, DWORD len);

    void (*run)(u8 *hash);
    void (*enter)(u8 *hash);
    void (*adv)();

    void (*push)(u8 *p, DWORD n);
    Buf (*pop)();
    Buf *(*top)();

    Frame *cur;
} Host;

static HINTERNET ses, con;
static u8 ZERO[H];

static Frame cur, ret[N];
static int active, rn;

static Ins ins[N];
static int insn;

static Ov ov[N];
static int ovn;

static Buf st[N];
static int sn;

void (*imp)();

static void root();
static void step();

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
    DWORD n = (DWORD)strlen(s);
    memset(o, 0, H);
    memcpy(o, s, n > H ? H : n);
}

void cvm_push(u8 *p, DWORD n)
{
    st[sn++] = B(p, n);
}

Buf cvm_pop()
{
    Buf z = {0};
    return sn ? st[--sn] : z;
}

Buf *cvm_top()
{
    return sn ? st + sn - 1 : 0;
}

void cvm_op(u8 *id, Op fn)
{
    memcpy(ins[insn].id, id, H);
    ins[insn++].fn = fn;
}

void cvm_op_name(char *name, Op fn)
{
    u8 id[H];
    T(name, id);
    cvm_op(id, fn);
}

void cvm_del(u8 *id)
{
    memcpy(ins[insn].id, id, H);
    ins[insn++].fn = 0;
}

void cvm_del_name(char *name)
{
    u8 id[H];
    T(name, id);
    cvm_del(id);
}

static Op opfind(u8 *id)
{
    for (int i = insn - 1; i >= 0; i--)
        if (!memcmp(ins[i].id, id, H))
            return ins[i].fn;
    return 0;
}

static Ov *ovfind(u8 *key)
{
    for (int i = ovn - 1; i >= 0; i--)
        if (!memcmp(ov[i].key, key, H))
            return ov + i;
    return 0;
}

void cvm_override(u8 *key, u8 *file, DWORD len)
{
    Ov *o = ovfind(key);
    if (!o)
    {
        o = ov + ovn++;
        memcpy(o->key, key, H);
    }
    free(o->f.p);
    o->f = B(file, len);
}

void cvm_touch()
{
    cvm_override(cur.key, cur.f.p, cur.f.n);
}

static Buf post(wchar_t *path, u8 *body, DWORD len)
{
    Buf b = {0};
    DWORD have, got;

    HINTERNET r = WinHttpOpenRequest(con, L"POST", path, 0, 0, 0, 0);

    WinHttpSendRequest(
        r,
        L"Content-Type: application/octet-stream\r\n",
        (DWORD)-1,
        body,
        len,
        len,
        0);

    WinHttpReceiveResponse(r, 0);

    while (WinHttpQueryDataAvailable(r, &have), have)
    {
        b.p = realloc(b.p, b.n + have);
        WinHttpReadData(r, b.p + b.n, have, &got);
        b.n += got;
    }

    WinHttpCloseHandle(r);
    return b;
}

static void firstchild(u8 *parent, u8 child[H])
{
    Buf r = post(L"/api/children", parent, H);
    memcpy(child, r.p + 4, H);
    free(r.p);
}

static Buf file(u8 *hash)
{
    return post(L"/api/file", hash, H);
}

static Buf keyfile(u8 *key)
{
    Ov *o = ovfind(key);
    u8 h[H];

    if (o)
        return B(o->f.p, o->f.n);

    firstchild(key, h);
    return file(h);
}

void cvm_adv()
{
    cur.off += H + U(cur.f.p + cur.off + H);
}

void cvm_enter(u8 *key)
{
    if (active)
    {
        cvm_adv();
        ret[rn++] = cur;
    }

    cur.f = keyfile(key);
    cur.off = 0;
    memcpy(cur.key, key, H);
    active = 1;
}

static void leave()
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
    {
        cvm_enter(h);
    }
}

static void root()
{
    cvm_run(ZERO);
    imp = step;
}

static void step()
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

static Host host = {
    cvm_op,
    cvm_op_name,
    cvm_del,
    cvm_del_name,

    cvm_override,
    cvm_touch,

    post,

    cvm_run,
    cvm_enter,
    cvm_adv,

    cvm_push,
    cvm_pop,
    cvm_top,

    &cur};

typedef void (*ModInit)(Host *);

static void loadmod(char *p)
{
    ((ModInit)GetProcAddress(LoadLibraryA(p), "cvm_init"))(&host);
}

static int cmpmod(const void *a, const void *b)
{
    return lstrcmpiA((char *)a, (char *)b);
}

static void loadmods()
{
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("mods\\*.dll", &fd);
    char ms[N][MAX_PATH];
    int n = 0;

    if (h == INVALID_HANDLE_VALUE)
        return;

    do
    {
        wsprintfA(ms[n++], "mods\\%s", fd.cFileName);
    } while (n < N && FindNextFileA(h, &fd));

    FindClose(h);

    qsort(ms, n, sizeof ms[0], cmpmod);

    for (int i = 0; i < n; i++)
        loadmod(ms[i]);
}

void boot()
{
    ses = WinHttpOpen(
        L"CVM",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        0,
        0,
        0);

    con = WinHttpConnect(
        ses,
        CVM_SERVER,
        CVM_PORT,
        0);

    loadmods();

    imp = root;
}

int main()
{
    boot();

    while (1)
        imp();
}