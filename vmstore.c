#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) SOCKET conn;
extern __declspec(dllimport) void cvm_firstchild(H p, H c);

static H id;
static int cache_on;
static u8 cache_raw[1<<20];
static u32 cache_len;
static H cache_key, cache_hash;

static void readn(void *b, u32 n) {
    u32 g = 0;
    while (g < n) {
        int r = recv(conn, (char*)b + g, n - g, 0);
        if (r < 1) exit(1);
        g += r;
    }
}

static void send_op(u8 op, const void *body, u32 len) {
    u8 h[5] = {op, len>>24, len>>16, len>>8, len};
    send(conn, (char*)h, 5, 0);
    if (len) send(conn, (char*)body, len, 0);
}

static u8 *recv_frame(u8 *st, u32 *n) {
    u8 h[5];
    readn(h, 5);
    *st = h[0];
    *n = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4];
    u8 *b = malloc(*n ? *n : 1);
    readn(b, *n);
    return b;
}

static void load_id(void) {
    H z = {0};
    if (memcmp(id, z, 32)) return;
    FILE *f = fopen("id.bin", "rb");
    if (f) { fread(id, 1, 32, f); fclose(f); }
}

static int same(const H a, const H b) { return !memcmp(a, b, 32); }

static int uget(const H k, H v) {
    u8 st, b[64], *r;
    u32 n;
    load_id();
    memcpy(b, id, 32);
    memcpy(b+32, k, 32);
    send_op(8, b, 64);
    r = recv_frame(&st, &n);
    if (!st && n >= 32) memcpy(v, r, 32);
    free(r);
    return !st;
}

static void uset(const H k, const H v) {
    u8 st, b[96], *r;
    u32 n;
    load_id();
    memcpy(b, id, 32);
    memcpy(b+32, k, 32);
    memcpy(b+64, v, 32);
    send_op(7, b, 96);
    r = recv_frame(&st, &n);
    free(r);
}

static void file_get(const H h, u8 **p, u32 *n) {
    u8 st;
    send_op(3, h, 32);
    *p = recv_frame(&st, n);
}

static void upload(const u8 *p, u32 n, H h) {
    u8 st, *r;
    u32 m;
    send_op(2, p, n);
    r = recv_frame(&st, &m);
    if (m >= 32) memcpy(h, r, 32);
    free(r);
}

__declspec(dllexport) int cvm_hash_same(const H a, const H b) { return same(a, b); }
__declspec(dllexport) u8 *cvm_cached_base(void) { return cache_raw; }
__declspec(dllexport) u32 cvm_cached_len(void) { return cache_len; }

__declspec(dllexport) void cvm_cache_flush(void) {
    H h;
    if (!cache_on) return;
    upload(cache_raw, cache_len, h);
    if (!same(h, cache_hash)) { uset(cache_key, h); memcpy(cache_hash, h, 32); }
}

__declspec(dllexport) void cvm_upload_async(const u8 *p, u32 n) {
    send_op(2, p, n);
}

__declspec(dllexport) void cvm_cache_load(const H k, const H h) {
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

__declspec(dllexport) int cvm_resolve_payload_hash(const H k, H h) {
    if (cache_on && same(k, cache_key)) { memcpy(h, cache_hash, 32); return 1; }
    if (!uget(k, h)) cvm_firstchild((u8*)k, h);
    cvm_cache_load(k, h);
    return 1;
}
