package main

import (
    "crypto/rand"
    "crypto/sha256"
    "encoding/binary"
    "encoding/gob"
    "encoding/json"
    "io"
    "log"
    "net"
    "net/http"
    "net/url"
    "os"
    "sort"
    "sync"
)

const (
    H       = 32
    MaxBody = 256 << 20

    OP_REGISTER byte = 1
    OP_UPLOAD   byte = 2
    OP_FILE     byte = 3
    OP_EDGE     byte = 4
    OP_CHILDREN byte = 5
    OP_VOTE     byte = 6
    OP_USET     byte = 7
    OP_UGET     byte = 8

    OK        byte = 0
    ERR_BAD   byte = 1
    ERR_DENY  byte = 2
    ERR_NF    byte = 3
    ERR_BIG   byte = 4
    ERR_INNER byte = 5
)

type Hash [H]byte
type Identity [H]byte

type UserKey struct {
    User Identity
    Key  Hash
}

type Edge struct {
    Parent Hash
    Child  Hash
}

type VoteKey struct {
    User   Identity
    Parent Hash
    Child  Hash
}

type DB struct {
    Files map[Hash][]byte
    Graph map[Hash][]Hash
    Users map[Identity]bool
    Vals  map[UserKey]Hash
    Score map[Edge]int64
    Voted map[VoteKey]bool
    Seq   map[Edge]int64
    Next  int64
}

type App struct {
    mu     sync.Mutex
    dbFile string
    secret string
    db     DB
}

func newDB() DB {
    return DB{
        Files: map[Hash][]byte{},
        Graph: map[Hash][]Hash{},
        Users: map[Identity]bool{},
        Vals:  map[UserKey]Hash{},
        Score: map[Edge]int64{},
        Voted: map[VoteKey]bool{},
        Seq:   map[Edge]int64{},
    }
}

func hashFrom(b []byte) Hash {
    var h Hash
    copy(h[:], b)
    return h
}

func idFrom(b []byte) Identity {
    var id Identity
    copy(id[:], b)
    return id
}

func hasChild(xs []Hash, h Hash) bool {
    for _, x := range xs {
        if x == h {
            return true
        }
    }
    return false
}

func (a *App) load() {
    a.db = newDB()

    f, err := os.Open(a.dbFile)
    if err != nil {
        return
    }
    defer f.Close()

    _ = gob.NewDecoder(f).Decode(&a.db)

    if a.db.Files == nil {
        a.db.Files = map[Hash][]byte{}
    }
    if a.db.Graph == nil {
        a.db.Graph = map[Hash][]Hash{}
    }
    if a.db.Users == nil {
        a.db.Users = map[Identity]bool{}
    }
    if a.db.Vals == nil {
        a.db.Vals = map[UserKey]Hash{}
    }
    if a.db.Score == nil {
        a.db.Score = map[Edge]int64{}
    }
    if a.db.Voted == nil {
        a.db.Voted = map[VoteKey]bool{}
    }
    if a.db.Seq == nil {
        a.db.Seq = map[Edge]int64{}
    }
}

func (a *App) save() {
    tmp := a.dbFile + ".tmp"

    f, err := os.Create(tmp)
    if err != nil {
        return
    }

    err = gob.NewEncoder(f).Encode(a.db)
    cerr := f.Close()

    if err == nil && cerr == nil {
        _ = os.Rename(tmp, a.dbFile)
    }
}

func (a *App) verifyTurnstile(token string) bool {
    if a.secret == "" {
        return true
    }

    form := url.Values{
        "secret":   {a.secret},
        "response": {token},
    }

    resp, err := http.PostForm(
        "https://challenges.cloudflare.com/turnstile/v0/siteverify",
        form,
    )
    if err != nil {
        return false
    }
    defer resp.Body.Close()

    var out struct {
        Success bool `json:"success"`
    }

    _ = json.NewDecoder(resp.Body).Decode(&out)
    return out.Success
}

func readFrame(c net.Conn) (byte, []byte, error) {
    var h [5]byte

    if _, err := io.ReadFull(c, h[:]); err != nil {
        return 0, nil, err
    }

    n := binary.BigEndian.Uint32(h[1:5])
    if n > MaxBody {
        return h[0], nil, io.ErrShortBuffer
    }

    b := make([]byte, n)
    if n != 0 {
        if _, err := io.ReadFull(c, b); err != nil {
            return 0, nil, err
        }
    }

    return h[0], b, nil
}

func writeFrame(c net.Conn, status byte, body []byte) error {
    var h [5]byte

    h[0] = status
    binary.BigEndian.PutUint32(h[1:5], uint32(len(body)))

    if _, err := c.Write(h[:]); err != nil {
        return err
    }

    if len(body) != 0 {
        _, err := c.Write(body)
        return err
    }

    return nil
}

func (a *App) handle(op byte, body []byte) (byte, []byte) {
    switch op {
    case OP_REGISTER:
        if !a.verifyTurnstile(string(body)) {
            return ERR_DENY, nil
        }

        var id Identity
        if _, err := rand.Read(id[:]); err != nil {
            return ERR_INNER, nil
        }

        a.mu.Lock()
        a.db.Users[id] = true
        a.save()
        a.mu.Unlock()

        return OK, id[:]

    case OP_UPLOAD:
        if len(body) == 0 {
            return ERR_BAD, nil
        }

        sum := sha256.Sum256(body)
        h := Hash(sum)

        a.mu.Lock()
        if _, exists := a.db.Files[h]; !exists {
            a.db.Files[h] = append([]byte(nil), body...)
            a.save()
        }
        a.mu.Unlock()

        return OK, h[:]

    case OP_FILE:
        if len(body) != 32 {
            return ERR_BAD, nil
        }

        h := hashFrom(body)

        a.mu.Lock()
        raw, found := a.db.Files[h]
        out := append([]byte(nil), raw...)
        a.mu.Unlock()

        if !found {
            return ERR_NF, nil
        }

        return OK, out

    case OP_EDGE:
        if len(body) != 64 {
            return ERR_BAD, nil
        }

        parent := hashFrom(body[:32])
        child := hashFrom(body[32:64])

        a.mu.Lock()
        if !hasChild(a.db.Graph[parent], child) {
            a.db.Graph[parent] = append(a.db.Graph[parent], child)
            a.save()
        }
        a.mu.Unlock()

        return OK, nil

    case OP_CHILDREN:
        if len(body) != 32 {
            return ERR_BAD, nil
        }

        parent := hashFrom(body)

        a.mu.Lock()

        list := append([]Hash(nil), a.db.Graph[parent]...)

        sort.SliceStable(list, func(i, j int) bool {
            ei := Edge{parent, list[i]}
            ej := Edge{parent, list[j]}

            if a.db.Score[ei] != a.db.Score[ej] {
                return a.db.Score[ei] > a.db.Score[ej]
            }

            return a.db.Seq[ei] > a.db.Seq[ej]
        })

        out := make([]byte, 4+len(list)*40)
        binary.BigEndian.PutUint32(out[:4], uint32(len(list)))

        off := 4
        for _, child := range list {
            copy(out[off:off+32], child[:])
            off += 32

            score := a.db.Score[Edge{parent, child}]
            binary.BigEndian.PutUint64(out[off:off+8], uint64(score))
            off += 8
        }

        a.mu.Unlock()

        return OK, out

    case OP_VOTE:
        if len(body) != 96 {
            return ERR_BAD, nil
        }

        user := idFrom(body[:32])
        parent := hashFrom(body[32:64])
        child := hashFrom(body[64:96])

        a.mu.Lock()
        defer a.mu.Unlock()

        if !a.db.Users[user] {
            return ERR_DENY, nil
        }

        if !hasChild(a.db.Graph[parent], child) {
            return ERR_BAD, nil
        }

        vk := VoteKey{user, parent, child}
        e := Edge{parent, child}

        if !a.db.Voted[vk] {
            a.db.Voted[vk] = true
            a.db.Score[e]++
        }

        a.db.Next++
        a.db.Seq[e] = a.db.Next

        a.save()

        return OK, nil

    case OP_USET:
        if len(body) != 96 {
            return ERR_BAD, nil
        }

        user := idFrom(body[:32])
        key := hashFrom(body[32:64])
        val := hashFrom(body[64:96])

        a.mu.Lock()
        defer a.mu.Unlock()

        if !a.db.Users[user] {
            return ERR_DENY, nil
        }

        a.db.Vals[UserKey{user, key}] = val
        a.save()

        return OK, nil

    case OP_UGET:
        if len(body) != 64 {
            return ERR_BAD, nil
        }

        user := idFrom(body[:32])
        key := hashFrom(body[32:64])

        a.mu.Lock()
        defer a.mu.Unlock()

        if !a.db.Users[user] {
            return ERR_DENY, nil
        }

        val, found := a.db.Vals[UserKey{user, key}]
        if !found {
            return ERR_NF, nil
        }

        return OK, val[:]
    }

    return ERR_BAD, nil
}

func (a *App) serve(c net.Conn) {
    defer c.Close()

    for {
        op, body, err := readFrame(c)
        if err != nil {
            return
        }

        status, out := a.handle(op, body)

        if writeFrame(c, status, out) != nil {
            return
        }
    }
}

func main() {
    app := &App{
        dbFile: "cvm.gob",
        secret: os.Getenv("CF_TURNSTILE_SECRET"),
    }

    app.load()

    ln, err := net.Listen("tcp", ":9000")
    if err != nil {
        panic(err)
    }

    log.Println("CVM binary TCP server listening on :9000")

    for {
        c, err := ln.Accept()
        if err == nil {
            go app.serve(c)
        }
    }
}

---

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

你要制作虚拟机首运行程序 即 "32 full zero token program" ，通过运行脚本程序来完成
我已有通过验证的id.bin

32 full zero 是一个自编辑器，图形可视化操作

一般情况下，指令会用*标准持续*来接续

标准持续函数应该是这样的
"""
ptr += 32
ptr += *(int*)ptr;
32字节 匹配有指令的话**直接执行**
否则
缓存>用户>公众 发现缓存有变需要进行用户覆盖
"""
"""
a b c d e... = hash + size

a
b
c
d
e
...
0000... - 32 full zero - 块结束标记 - 一般程序会在块结束标记之前就已经返回或停止等等行为，所以块结束标记一般是不会被执行的
"""
标准持续是一个规定标准,目的是最简单的可以知道一个块的指令执行顺序，这样编辑器才可以知道如何下手

***新增指令必须从下到上《从基本通用的到高级专用的》，Token 语义必须平台中立***