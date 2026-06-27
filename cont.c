#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef void (*Fn)();

extern SOCKET conn;
extern void (*imp)();
extern Fn find(H h);
extern void cvm_firstchild(H p, H c);

u8 *ptr;
H id;

static void readn(void *b, u32 n) { u32 g = 0; while (g < n) g += recv(conn, (char*)b + g, n - g, 0); }
static void send_op(u8 op, void *body, u32 len) { u8 h[5] = {op, len>>24, len>>16, len>>8, len}; send(conn, (char*)h, 5, 0); if (len) send(conn, (char*)body, len, 0); }
static u8 *recv_frame(u8 *st, u32 *n) { u8 h[5]; readn(h, 5); *st = h[0]; *n = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4]; u8 *b = malloc(*n); readn(b, *n); return b; }

static void load_id() { FILE *f = fopen("id.bin", "rb"); fread(id, 1, 32, f); fclose(f); }
static int same(H a, H b) { return !memcmp(a, b, 32); }
static int zero(H h) { H z = {0}; return same(h, z); }

static int uget(H k, H v) { u8 st, b[64], *r; u32 n; memcpy(b, id, 32); memcpy(b+32, k, 32); send_op(8, b, 64); r = recv_frame(&st, &n); if (!st) memcpy(v, r, 32); free(r); return !st; }
static void uset(H k, H v) { u8 st, b[96], *r; u32 n; memcpy(b, id, 32); memcpy(b+32, k, 32); memcpy(b+64, v, 32); send_op(7, b, 96); r = recv_frame(&st, &n); free(r); }
static void file_get(H h, u8 **p, u32 *n) { u8 st; send_op(3, h, 32); *p = recv_frame(&st, n); }
static void upload(u8 *p, u32 n, H h) { u8 st, *r; u32 m; send_op(2, p, n); r = recv_frame(&st, &m); memcpy(h, r, 32); free(r); }

static int cache_on;
static u8 cache_raw[1<<20];
static u32 cache_len;
static H cache_key, cache_hash;

static void cache_flush() {
    H h;
    if (!cache_on) return;
    upload(cache_raw, cache_len, h);
    if (!same(h, cache_hash)) { uset(cache_key, h); memcpy(cache_hash, h, 32); }
}

static void cache_load(H k, H h) {
    u8 *p;
    memcpy(cache_key, k, 32);
    memcpy(cache_hash, h, 32);
    file_get(h, &p, &cache_len);
    memcpy(cache_raw, p, cache_len);
    free(p);
    cache_on = 1;
}

void cont() {
    H k, h;
    Fn f;

    if (zero(id)) load_id();
    cache_flush();

    ptr += 32 + *(int*)(ptr + 32);
    memcpy(k, ptr, 32);

    f = find(k);
    if (f) { imp = f; return; }

    if (cache_on && same(k, cache_key)) memcpy(h, cache_hash, 32);
    else {
        if (!uget(k, h)) cvm_firstchild(k, h);
        cache_load(k, h);
    }

    f = find(h);
    if (!f) { cvm_firstchild(h, k); f = find(k); }
    imp = f;
}
