#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef void (*Fn)();

extern __declspec(dllimport) SOCKET conn;
extern __declspec(dllimport) Fn imp;
extern __declspec(dllimport) Fn find(H h);
extern __declspec(dllimport) void cvm_firstchild(H p, H c);

__declspec(dllexport) u8 *ptr;
H id;

static void readn(void *b, u32 n) { u32 g = 0; while (g < n) { int r = recv(conn, (char*)b + g, n - g, 0); if (r < 1) exit(1); g += r; } }
static void send_op(u8 op, const void *body, u32 len) { u8 h[5] = {op, len>>24, len>>16, len>>8, len}; send(conn, (char*)h, 5, 0); if (len) send(conn, (char*)body, len, 0); }
static u8 *recv_frame(u8 *st, u32 *n) { u8 h[5]; readn(h, 5); *st = h[0]; *n = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4]; u8 *b = malloc(*n ? *n : 1); readn(b, *n); return b; }

static void load_id() { FILE *f = fopen("id.bin", "rb"); if (f) { fread(id, 1, 32, f); fclose(f); } }
static int same(const H a, const H b) { return !memcmp(a, b, 32); }
static int zero(const H h) { H z = {0}; return same(h, z); }

static int uget(const H k, H v) { u8 st, b[64], *r; u32 n; memcpy(b, id, 32); memcpy(b+32, k, 32); send_op(8, b, 64); r = recv_frame(&st, &n); if (!st && n >= 32) memcpy(v, r, 32); free(r); return !st; }
static void uset(const H k, const H v) { u8 st, b[96], *r; u32 n; memcpy(b, id, 32); memcpy(b+32, k, 32); memcpy(b+64, v, 32); send_op(7, b, 96); r = recv_frame(&st, &n); free(r); }
static void file_get(const H h, u8 **p, u32 *n) { u8 st; send_op(3, h, 32); *p = recv_frame(&st, n); }
static void upload(const u8 *p, u32 n, H h) { u8 st, *r; u32 m; send_op(2, p, n); r = recv_frame(&st, &m); if (m >= 32) memcpy(h, r, 32); free(r); }
static void upload_async(const u8 *p, u32 n) { send_op(2, p, n); }

static int cache_on;
static u8 cache_raw[1<<20];
static u32 cache_len;
static H cache_key, cache_hash;
static u8 *cur_base;
static H cur_key;

__declspec(dllexport) u8 *cvm_payload(void) { return ptr + 36; }
__declspec(dllexport) u32 cvm_payload_size(void) { return *(u32*)(ptr + 32); }
__declspec(dllexport) u8 *cvm_token(void) { return ptr; }

static void cache_flush() {
    H h;
    if (!cache_on) return;
    upload(cache_raw, cache_len, h);
    if (!same(h, cache_hash)) { uset(cache_key, h); memcpy(cache_hash, h, 32); }
}

static void cache_load(const H k, const H h) {
    u8 *p;
    u32 n;
    memcpy(cache_key, k, 32);
    memcpy(cache_hash, h, 32);
    file_get(h, &p, &n);
    if (n > sizeof(cache_raw)) n = sizeof(cache_raw);
    memcpy(cache_raw, p, n);
    cache_len = n;
    free(p);
    cache_on = 1;
}

static int resolve_payload_hash(const H k, H h) {
    if (cache_on && same(k, cache_key)) { memcpy(h, cache_hash, 32); return 1; }
    if (!uget(k, h)) cvm_firstchild((u8*)k, h);
    cache_load(k, h);
    return 1;
}

static void start_fn(Fn f, const H k, u8 *base) {
    if (k) memcpy(cur_key, k, 32);
    cur_base = base;
    ptr = base;
    imp = f;
}

__declspec(dllexport) void cvm_exec(const H k) {
    H h, child;
    Fn f;

    if (zero(id)) load_id();
    cache_flush();

    f = find((u8*)k);
    if (f) { start_fn(f, k, cur_base); return; }

    resolve_payload_hash(k, h);
    f = find(h);
    if (!f) { cvm_firstchild(h, child); f = find(child); }
    start_fn(f, k, cache_raw);
}

__declspec(dllexport) void cvm_exec_payload(H k) {
    H oldh, newh;
    u32 n = cvm_payload_size();
    u8 *p = cvm_payload();

    if (n >= 32) memcpy(k, p, 32);
    resolve_payload_hash(k, oldh);
    if (!same(oldh, k) && n >= 32) {
        memcpy(p, oldh, 32);
        memcpy(k, oldh, 32);
        upload_async(cur_base, cache_len);
    }
    cvm_exec(k);
}

__declspec(dllexport) void cvm_reexec(void) {
    Fn f = find(cur_key);
    if (f) { ptr = cur_base; imp = f; return; }
    if (cur_base) ptr = cur_base;
}

__declspec(dllexport) void cont() {
    H k;

    if (zero(id)) load_id();
    cache_flush();

    ptr += 32 + *(u32*)(ptr + 32);
    memcpy(k, ptr, 32);
    cvm_exec(k);
}
