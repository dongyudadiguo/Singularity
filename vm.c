// CVM — 极简虚拟机
// gcc vm.c -Os -s -o cvm.exe -lws2_32
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define CVM_SERVER "118.25.42.70"
#define CVM_PORT 9000
#define H 32
#define N 256
#define OP_FILE 3
#define OP_CHILDREN 5
typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;
typedef void (*Op)(u8 *, uint32_t);
typedef struct { u8 id[H]; Op fn; } Ins;
typedef struct { u8 key[H]; Buf f; } Ov;

// ——— 全局 VM 状态 ———
static SOCKET sock;
static uint32_t off;              // 当前执行位置
static Buf cur;                   // 当前块数据
static u8 key[H];                 // 当前块 hash
static Buf st[N]; static int sn;  // 数据栈
static Ins ins[N]; static int in; // 指令表
static Ov ov[N]; static int on;   // OV 缓存

void (*imp)(void);                // 主循环调用目标

// ——— 当前指令缓冲区 ———
static Op cur_fn;
static u8 *cur_pay; static uint32_t cur_len;

static void tramp(void) { cur_fn(cur_pay, cur_len); }

// ——— 工具 ———
static uint32_t U(u8 *p) { uint32_t x; memcpy(&x, p, 4); return x; }
static int Z(u8 *p) { for (int i = 0; i < H; i++) if (p[i]) return 0; return 1; }
static Buf B(u8 *p, DWORD n) { Buf b = {malloc(n), n}; if (n) memcpy(b.p, p, n); return b; }

// ——— 网络 ———
static int xfer(u8 *p, int n, int rd) {
    while (n) { int k = rd ? recv(sock, (char *)p, n, 0) : send(sock, (char *)p, n, 0);
        if (k <= 0) { sock = INVALID_SOCKET; return 0; } p += k; n -= k; }
    return 1;
}
Buf cvm_rpc(uint8_t op, u8 *body, DWORD len) {
    u8 h[5]; Buf b = {0}; uint32_t be = htonl(len);
    h[0] = op; memcpy(h + 1, &be, 4);
    if (!xfer(h, 5, 0) || (len && !xfer(body, len, 0)) || !xfer(h, 5, 1)) return b;
    b.n = ntohl(*(uint32_t *)(h + 1)); b.p = malloc(b.n);
    if (b.n && !xfer(b.p, b.n, 1)) { free(b.p); b.p = 0; b.n = 0; }
    return b;
}

// ——— 栈 ———
void cvm_push(u8 *p, uint32_t n) { st[sn++] = B(p, n); }
Buf cvm_pop(void) { Buf z = {0}; return sn ? st[--sn] : z; }
Buf *cvm_top(void) { return sn ? st + sn - 1 : 0; }

// ——— 指令表 ———
void cvm_op(u8 *id, Op fn) { memcpy(ins[in].id, id, H); ins[in++].fn = fn; }
void cvm_op_name(char *s, Op fn) { u8 id[H]; DWORD n = strlen(s);
    memset(id, 0, H); memcpy(id, s, n > H ? H : n); cvm_op(id, fn); }
void cvm_del(u8 *id) { memcpy(ins[in].id, id, H); ins[in++].fn = 0; }
void cvm_del_name(char *s) { u8 id[H]; DWORD n = strlen(s);
    memset(id, 0, H); memcpy(id, s, n > H ? H : n); cvm_del(id); }

static Op opfind(u8 *id) {
    for (int i = in - 1; i >= 0; i--) if (!memcmp(ins[i].id, id, H)) return ins[i].fn;
    return 0;
}

// ——— OV ———
static Ov *ovfind(u8 *k) {
    for (int i = on - 1; i >= 0; i--) if (!memcmp(ov[i].key, k, H)) return ov + i;
    return 0;
}
void cvm_override(u8 *k, u8 *f, DWORD n) { Ov *o = ovfind(k);
    if (!o) { o = ov + on++; memcpy(o->key, k, H); } free(o->f.p); o->f = B(f, n); }
void cvm_touch(void) { cvm_override(key, cur.p, cur.n); }

// ——— 块文件: OV 缓存 > 服务器(CHILDREN[0]→FILE) ———
static Buf getfile(u8 *k) {
    Ov *o = ovfind(k);
    if (o) return B(o->f.p, o->f.n);
    u8 h[H] = {0}; Buf r = cvm_rpc(OP_CHILDREN, k, H);
    if (r.p && r.n >= 36) memcpy(h, r.p + 4, H);
    free(r.p);
    return cvm_rpc(OP_FILE, h, H);
}

// ——— 标准持续: ptr += 32 + *(int*)ptr ———
static void adv(void) { off += H + U(cur.p + off + H); }

// ——— resolve: 取32字节 → 匹配指令；否则 getfirstchild → 递归 ———
static Op resolve(u8 *tok) {
    Op fn = opfind(tok);
    if (fn) return fn;
    Buf child = getfile(tok);
    if (!child.p || child.n < H) { free(child.p); return 0; }
    fn = opfind(child.p);
    free(child.p);
    return fn;
}

// ——— schedule: 解析当前位置 → 设置 imp ———
static void schedule(void) {
    if (off + H > cur.n || Z(cur.p + off)) { imp = 0; return; }
    cur_fn = resolve(cur.p + off);
    cur_pay = cur.p + off + H + 4;
    cur_len = U(cur.p + off + H) - 4;
    imp = tramp;
}

// ——— 启动 ———
static void entry(void) {
    u8 ZERO[H] = {0};
    free(cur.p);
    cur = getfile(ZERO);
    memcpy(key, ZERO, H);
    off = 0;
    schedule();
}

void boot(void) {
    WSADATA w; struct sockaddr_in a = {0}; int to = 5000;
    WSAStartup(MAKEWORD(2, 2), &w);
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    a.sin_family = AF_INET; a.sin_port = htons(CVM_PORT);
    a.sin_addr.s_addr = inet_addr(CVM_SERVER);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&to, sizeof to);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&to, sizeof to);
    if (connect(sock, (struct sockaddr *)&a, sizeof a) == SOCKET_ERROR) {
        closesocket(sock); sock = INVALID_SOCKET;
    }
    {
        WIN32_FIND_DATAA fd; char ms[N][MAX_PATH]; int n = 0;
        HANDLE hf = FindFirstFileA("mods\\*.dll", &fd);
        if (hf != INVALID_HANDLE_VALUE) {
            do wsprintfA(ms[n++], "mods\\%s", fd.cFileName);
            while (n < N && FindNextFileA(hf, &fd));
            FindClose(hf);
            for (int i = 0; i < n - 1; i++)
                for (int j = i + 1; j < n; j++)
                    if (lstrcmpiA(ms[i], ms[j]) > 0) { char t[MAX_PATH]; lstrcpyA(t, ms[i]); lstrcpyA(ms[i], ms[j]); lstrcpyA(ms[j], t); }
            for (int i = 0; i < n; i++) {
                HMODULE m = LoadLibraryA(ms[i]);
                if (m) { void (*init)(void *); *(void**)&init = GetProcAddress(m, "cvm_init");
                    if (init) {
                        void *host[15] = {cvm_op, cvm_op_name, cvm_del, cvm_del_name, cvm_override, cvm_touch, cvm_rpc, 0, 0, adv, cvm_push, cvm_pop, cvm_top, 0};
                        init(host);
                    }
                }
            }
        }
    }
    entry();
}

int main()
{
boot();
    while (1)
    {
        imp();
    }
}