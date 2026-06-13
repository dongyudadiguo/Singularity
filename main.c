// Win11 + MinGW-w64 x86/x64
// VM 本体无预设指令；指令都从 mods/*.dll 动态加载

#include <windows.h>
#include <winhttp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define H 32
#define N 256

typedef unsigned char u8;

typedef struct { u8 *p; DWORD n; } Buf;
typedef struct { Buf f; DWORD off; u8 key[H]; } Frame;
typedef int (*Op)(u8 *data, uint32_t len);

typedef struct { u8 id[H]; Op fn; } Ins;
typedef struct { u8 key[H]; Buf f; } Ov;

typedef struct Host {
    void (*op)(u8 *id, Op fn);             // 注册指令
    void (*op_name)(char *name, Op fn);    // 注册字符串指令
    void (*del)(u8 *id);                   // 删除指令
    void (*del_name)(char *name);          // 删除字符串指令

    void (*override)(u8 *key, u8 *file, DWORD len);
    void (*touch)();

    Buf  (*post)(wchar_t *path, u8 *body, DWORD len);

    void (*run)(u8 *hash);                 // runtheblock(hash)
    void (*enter)(u8 *hash);               // 直接进入 hash 对应文件
    void (*adv)();                         // 标准持续逻辑

    Frame *cur;                            // 当前块现场
} Host;

static HINTERNET ses, con;
static u8 ZERO[H];

static Frame cur, ret[N];
static int active, rn;

static Ins ins[N];
static int insn;

static Ov ov[N];
static int ovn;

void (*imp)();

static void imp_root();
static void imp_step();

static uint32_t u32(u8 *p) { return *(uint32_t *)p; }

static int zero(u8 *p) {
    for (int i = 0; i < H; i++) if (p[i]) return 0;
    return 1;
}

static Buf clone(u8 *p, DWORD n) {
    Buf b = { malloc(n), n };
    memcpy(b.p, p, n);
    return b;
}

static void tok(char *s, u8 out[H]) {
    memset(out, 0, H);
    memcpy(out, s, strlen(s) > H ? H : strlen(s));
}

void cvm_op(u8 *id, Op fn) {
    memcpy(ins[insn].id, id, H);
    ins[insn++].fn = fn;
}

void cvm_op_name(char *name, Op fn) {
    u8 id[H];
    tok(name, id);
    cvm_op(id, fn);
}

void cvm_del(u8 *id) {
    memcpy(ins[insn].id, id, H);
    ins[insn++].fn = 0;        // tombstone：后注册优先，所以这就是删除
}

void cvm_del_name(char *name) {
    u8 id[H];
    tok(name, id);
    cvm_del(id);
}

static Op findop(u8 *id) {
    for (int i = insn - 1; i >= 0; i--)
        if (!memcmp(ins[i].id, id, H)) return ins[i].fn;
    return 0;
}

static Ov *findov(u8 *key) {
    for (int i = ovn - 1; i >= 0; i--)
        if (!memcmp(ov[i].key, key, H)) return ov + i;
    return 0;
}

void cvm_override(u8 *key, u8 *file, DWORD len) {
    Ov *o = findov(key);
    if (!o) o = ov + ovn++, memcpy(o->key, key, H);
    free(o->f.p);
    o->f = clone(file, len);
}

void cvm_touch() {
    cvm_override(cur.key, cur.f.p, cur.f.n);
}

static Buf post(wchar_t *path, u8 *body, DWORD len) {
    Buf b = {0};
    DWORD have, got;

    HINTERNET req = WinHttpOpenRequest(con, L"POST", path, 0, 0, 0, 0);

    WinHttpSendRequest(
        req,
        L"Content-Type: application/octet-stream",
        -1,
        body,
        len,
        len,
        0
    );

    WinHttpReceiveResponse(req, 0);

    while (WinHttpQueryDataAvailable(req, &have), have) {
        b.p = realloc(b.p, b.n + have);
        WinHttpReadData(req, b.p + b.n, have, &got);
        b.n += got;
    }

    WinHttpCloseHandle(req);
    return b;
}

static void firstchild(u8 *parent, u8 *child) {
    Buf r = post(L"/api/children", parent, H);
    memcpy(child, r.p + 4, H);
    free(r.p);
}

static Buf file(u8 *hash) {
    return post(L"/api/file", hash, H);
}

static Buf keyfile(u8 *key) {
    Ov *o = findov(key);
    u8 h[H];

    if (o) return clone(o->f.p, o->f.n);

    firstchild(key, h);
    return file(h);
}

// 标准持续逻辑：ptr += 32 + *(uint32_t *)(ptr + 32)
void cvm_adv() {
    cur.off += H + u32(cur.f.p + cur.off + H);
}

void cvm_enter(u8 *key) {
    if (active) ret[rn++] = cur;

    cur.f = keyfile(key);
    cur.off = 0;
    memcpy(cur.key, key, H);
    active = 1;
}

static void leave() {
    free(cur.f.p);

    if (!rn) {
        active = 0;
        return;
    }

    cur = ret[--rn];
    cvm_adv();
}

// runtheblock(hash)
void cvm_run(u8 *h) {
    Op op = findop(h);

    if (op) {
        u8 *p = cur.f.p + cur.off;
        if (!op(p + H + 4, u32(p + H) - 4)) cvm_adv();
    } else {
        cvm_enter(h);
    }
}

static void imp_root() {
    cvm_run(ZERO);       // runtheblock<full zero>
    imp = imp_step;
}

static void imp_step() {
    if (!active) {
        imp = imp_root;
        return;
    }

    if (cur.off >= cur.f.n || zero(cur.f.p + cur.off)) {
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
    &cur
};

typedef void (*ModInit)(Host *);

static void loadmod(char *p) {
    ModInit init = (ModInit)GetProcAddress(LoadLibraryA(p), "cvm_init");
    init(&host);
}

// mods 按文件名排序加载：00_x.dll -> 10_x.dll -> 20_x.dll
static int cmpmod(const void *a, const void *b) {
    return lstrcmpiA((char *)a, (char *)b);
}

static void loadmods() {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("mods\\*.dll", &fd);
    char ms[N][MAX_PATH];
    int n = 0;

    if (h == INVALID_HANDLE_VALUE) return;

    do {
        wsprintfA(ms[n++], "mods\\%s", fd.cFileName);
    } while (FindNextFileA(h, &fd));

    FindClose(h);

    qsort(ms, n, MAX_PATH, cmpmod);

    for (int i = 0; i < n; i++) loadmod(ms[i]);
}

void boot() {
    memset(ZERO, 0, H);

    ses = WinHttpOpen(L"CVM", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    con = WinHttpConnect(ses, L"124.221.146.23", 9000, 0);

    loadmods();     // 指令都在这里动态进入 VM

    imp = imp_root;
}

int main() {
    boot();

    while (1) {
        imp();
    }
}