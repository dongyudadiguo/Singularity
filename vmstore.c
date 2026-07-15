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
 *   - a multi-entry LRU in-process block cache (32 slots)
 *   - non-blocking write-back when cached bytes no longer match cache_hash
 */

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) SOCKET conn;
extern __declspec(dllimport) void cvm_firstchild(H p, H c);

static H id;

/* Multi-entry LRU cache */
#define CACHE_SLOTS 256

typedef struct {
    H key;
    H hash;
    u8 data[1<<20];
    u32 len;
    int on;
    int pin;   /* >0: live execution frame — do not LRU-evict */
    u32 lru;
} CacheSlot;

static CacheSlot slots[CACHE_SLOTS];
static int primary_idx = 0;
static u32 lru_counter = 0;

/* Networking helpers */

static int readn_sock(SOCKET s, void *b, u32 n) {
    u32 g = 0;
    while (g < n) {
        int r = recv(s, (char*)b + g, n - g, 0);
        if (r < 1) return 0;
        g += r;
    }
    return 1;
}

static int readn(void *b, u32 n) { return readn_sock(conn, b, n); }

static void send_op_sock(SOCKET s, u8 op, const void *body, u32 len) {
    u8 h[5] = {op, len>>24, len>>16, len>>8, len};
    send(s, (char*)h, 5, 0);
    if (len) send(s, (char*)body, len, 0);
}

static void send_op(u8 op, const void *body, u32 len) { send_op_sock(conn, op, body, len); }

static u8 *recv_frame_sock(SOCKET s, u8 *st, u32 *n) {
    u8 h[5];
    if (!readn_sock(s, h, 5)) { *st = 1; *n = 0; return (u8*)calloc(1, 1); }
    *st = h[0];
    *n = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4];
    u8 *b = malloc(*n ? *n : 1);
    if (!b) { *st = 1; *n = 0; return (u8*)calloc(1, 1); }
    if (*n && !readn_sock(s, b, *n)) { free(b); *st = 1; *n = 0; return (u8*)calloc(1, 1); }
    return b;
}

static u8 *recv_frame(u8 *st, u32 *n) { return recv_frame_sock(conn, st, n); }

static void load_id(void) {
    H z = {0};
    if (memcmp(id, z, 32)) return;
    FILE *f = fopen("id.bin", "rb");
    if (f) { fread(id, 1, 32, f); fclose(f); }
    CreateDirectoryA("cache", NULL);
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
    char path[120];
    snprintf(path, sizeof(path),
        "cache/%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x.bin",
        h[0],h[1],h[2],h[3],h[4],h[5],h[6],h[7],
        h[8],h[9],h[10],h[11],h[12],h[13],h[14],h[15],
        h[16],h[17],h[18],h[19],h[20],h[21],h[22],h[23],
        h[24],h[25],h[26],h[27],h[28],h[29],h[30],h[31]);
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        *n = (u32)ftell(f);
        fseek(f, 0, SEEK_SET);
        *p = (u8*)malloc(*n ? *n : 1);
        if (*p) fread(*p, 1, *n, f);
        fclose(f);
        return;
    }
    u8 st;
    send_op(3, h, 32);
    *p = recv_frame(&st, n);
    f = fopen(path, "wb");
    if (f) { fwrite(*p, 1, *n, f); fclose(f); }
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

__declspec(dllexport) void cvm_cache_pin_base(const u8 *base) {
    if (!base) return;
    for (int i = 0; i < CACHE_SLOTS; i++) {
        if (slots[i].on && slots[i].data == base) {
            slots[i].pin++;
            return;
        }
    }
}

__declspec(dllexport) void cvm_cache_unpin_base(const u8 *base) {
    if (!base) return;
    for (int i = 0; i < CACHE_SLOTS; i++) {
        if (slots[i].on && slots[i].data == base) {
            if (slots[i].pin > 0) slots[i].pin--;
            return;
        }
    }
}

__declspec(dllexport) void cvm_cache_load(const H k, const H h) {
    u8 *p;
    u32 n;
    int target = -1;
    /* Prefer free slot. */
    for (int i = 0; i < CACHE_SLOTS; i++) {
        if (!slots[i].on) { target = i; break; }
    }
    /* Else LRU among unpinned slots only — never clobber live frames. */
    if (target < 0) {
        int best = -1;
        for (int i = 0; i < CACHE_SLOTS; i++) {
            if (slots[i].pin) continue;
            if (best < 0 || slots[i].lru < slots[best].lru) best = i;
        }
        target = best;
    }
    /* All pinned: refuse to overwrite; keep primary on a hit of k if any. */
    if (target < 0) {
        if (cvm_cache_hit(k)) {
            memcpy((void*)h, slots[primary_idx].hash, 32);
        }
        return;
    }
    memcpy(slots[target].key, k, 32);
    memcpy(slots[target].hash, h, 32);
    file_get(h, &p, &n);
    if (n > sizeof(slots[0].data)) n = sizeof(slots[0].data);
    memcpy(slots[target].data, p, n);
    slots[target].len = n;
    free(p);
    slots[target].on = 1;
    slots[target].pin = 0;
    slots[target].lru = ++lru_counter;
    primary_idx = target;
}


/* Query whether the current identity has a user override for token k.
 * Returns 1 and writes the override hash into h when present; else 0.
 * Cached (256 slots, including negatives) — editor color query is hot. */
#define OV_CACHE 256
typedef struct { H key; H val; int on; int hit; } OvSlot;
static OvSlot ov_slots[OV_CACHE];

static u32 ov_hash(const H k) {
    return (u32)k[0] | ((u32)k[1] << 8) | ((u32)k[2] << 16) | ((u32)k[3] << 24);
}

__declspec(dllexport) int cvm_has_override(const H k, H h) {
    u32 idx = ov_hash(k) & (OV_CACHE - 1);
    for (u32 n = 0; n < OV_CACHE; n++) {
        u32 i = (idx + n) & (OV_CACHE - 1);
        if (ov_slots[i].on && same(ov_slots[i].key, k)) {
            if (ov_slots[i].hit) {
                if (h) memcpy(h, ov_slots[i].val, 32);
                return 1;
            }
            return 0;
        }
        if (!ov_slots[i].on) {
            H v;
            int ok = uget(k, v);
            memcpy(ov_slots[i].key, k, 32);
            ov_slots[i].on = 1;
            ov_slots[i].hit = ok ? 1 : 0;
            if (ok) {
                memcpy(ov_slots[i].val, v, 32);
                if (h) memcpy(h, v, 32);
                return 1;
            }
            return 0;
        }
    }
    /* full: uncached */
    {
        H v;
        if (!uget(k, v)) return 0;
        if (h) memcpy(h, v, 32);
        return 1;
    }
}

__declspec(dllexport) void cvm_override_cache_invalidate(const H k) {
    if (!k) {
        for (int i = 0; i < OV_CACHE; i++) ov_slots[i].on = 0;
        return;
    }
    for (int i = 0; i < OV_CACHE; i++) {
        if (ov_slots[i].on && same(ov_slots[i].key, k)) ov_slots[i].on = 0;
    }
}

extern __declspec(dllexport) u32 cvm_children(const H parent, H *out, u32 cap);

__declspec(dllexport) int cvm_resolve_payload_hash(const H k, H h) {
    /* Hot path: every bare logical token / views row resolve hits this.
     * Cold miss used to always uget() on the main conn (full RTT, ~1s feel)
     * and 32-slot LRU thrashed when switching pan <-> rmb actions.
     * Path: block cache -> override cache (hit/miss) -> children disk cache
     * -> network firstchild only if needed -> load bytes (disk file first). */
    if (cvm_cache_hit(k)) {
        memcpy(h, slots[primary_idx].hash, 32);
        { static int _vc = 0; if (++_vc >= 512) { _vc = 0; cvm_cache_verify_async(); } }
        return 1;
    }
    if (!cvm_has_override(k, h)) {
        H kid;
        if (cvm_children(k, &kid, 1) >= 1) memcpy(h, kid, 32);
        else cvm_firstchild((u8*)k, h);
    }
    cvm_cache_load(k, h);
    return 1;
}

__declspec(dllexport) u32 cvm_children(const H parent, H *out, u32 cap) {
    char path[140];
    snprintf(path, sizeof(path),
        "cache/%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x"
        "%02x%02x%02x%02x%02x%02x%02x%02x_ch.bin",
        parent[0],parent[1],parent[2],parent[3],parent[4],parent[5],parent[6],parent[7],
        parent[8],parent[9],parent[10],parent[11],parent[12],parent[13],parent[14],parent[15],
        parent[16],parent[17],parent[18],parent[19],parent[20],parent[21],parent[22],parent[23],
        parent[24],parent[25],parent[26],parent[27],parent[28],parent[29],parent[30],parent[31]);
    /* Try disk cache */
    FILE *fc = fopen(path, "rb");
    if (fc) {
        u32 cnt = 0;
        if (fread(&cnt, 4, 1, fc) == 1) {
            u32 got = cnt < cap ? cnt : cap;
            for (u32 i = 0; i < got; i++)
                if (fread(out[i], 1, 32, fc) != 32) { cnt = i; break; }
            fclose(fc);
            return cnt;
        }
        fclose(fc);
    }
    /* Network fetch */
    u8 st, *r; u32 n, cnt = 0;
    send_op(5, parent, 32);
    r = recv_frame(&st, &n);
    u32 total = 0;
    if (!st && n >= 4) {
        cnt = ((u32)r[0]<<24)|((u32)r[1]<<16)|((u32)r[2]<<8)|r[3];
        if (cnt > (n-4)/40) cnt = (n-4)/40;
        total = cnt < 256 ? cnt : 256;
        u32 got = cnt < cap ? cnt : cap;
        for (u32 i = 0; i < got; i++) memcpy(out[i], r+4+i*40, 32);
    }
    /* Save to cache (before freeing r) */
    fc = fopen(path, "wb");
    if (fc) {
        fwrite(&cnt, 4, 1, fc);
        for (u32 i = 0; i < total; i++)
            fwrite(r+4+i*40, 1, 32, fc);
        fclose(fc);
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


/* === hand-run arm table (process-local, not persisted) === */
#define HAND_CAP 256
typedef struct { u32 uid; u8 armed; u8 on; } HandEnt;
static HandEnt g_hand[HAND_CAP];

static int hand_find(u32 uid, int create) {
    int free_i = -1;
    if (!uid) return -1;
    for (int i = 0; i < HAND_CAP; i++) {
        if (g_hand[i].on && g_hand[i].uid == uid) return i;
        if (!g_hand[i].on && free_i < 0) free_i = i;
    }
    if (!create) return -1;
    if (free_i < 0) free_i = 0;
    g_hand[free_i].on = 1;
    g_hand[free_i].uid = uid;
    g_hand[free_i].armed = 0;
    return free_i;
}

__declspec(dllexport) int cvm_hand_armed(u32 uid) {
    int i = hand_find(uid, 0);
    return (i >= 0 && g_hand[i].armed) ? 1 : 0;
}

__declspec(dllexport) void cvm_hand_toggle(u32 uid) {
    int i = hand_find(uid, 1);
    if (i >= 0) g_hand[i].armed ^= 1;
}

__declspec(dllexport) void cvm_hand_set(u32 uid, int armed) {
    int i = hand_find(uid, 1);
    if (i >= 0) g_hand[i].armed = armed ? 1 : 0;
}

/* === editor support: dirty / vote / heat / flush_key === */

__declspec(dllexport) int cvm_cache_dirty(void) {
    CacheSlot *s = &slots[primary_idx];
    H cur;
    if (!s->on) return 0;
    if (!sha256(s->data, s->len, cur)) return 0;
    return !same(cur, s->hash);
}

/* Resolve key into primary cache, then report dirty vs last-flushed hash. */
__declspec(dllexport) int cvm_key_dirty(const H key) {
    H h;
    if (!key) return 0;
    cvm_resolve_payload_hash(key, h);
    {
        CacheSlot *s = &slots[primary_idx];
        H cur;
        if (!s->on || !same(s->key, key)) return 0;
        if (!sha256(s->data, s->len, cur)) return 0;
        return !same(cur, s->hash);
    }
}

__declspec(dllexport) void cvm_flush_key(const H key) {
    H h;
    if (!key) return;
    cvm_resolve_payload_hash(key, h);
    {
        CacheSlot *s = &slots[primary_idx];
        if (!s->on || !same(s->key, key)) return;
        upload(s->data, s->len, h);
        if (!same(h, s->hash)) {
            uset(s->key, h);
            memcpy(s->hash, h, 32);
            cvm_override_cache_invalidate(s->key);
        }
    }
}

/* OP_VOTE = 6; body: user[32]+parent[32]+child[32] */
__declspec(dllexport) void cvm_vote(const H parent, const H child) {
    u8 st, b[96], *r;
    u32 n;
    load_id();
    memcpy(b, id, 32);
    memcpy(b + 32, parent, 32);
    memcpy(b + 64, child, 32);
    send_op(6, b, 96);
    r = recv_frame(&st, &n);
    free(r);
}

/* Heat: per-uid brightness with ~1s decay; also track node keys. */
#define HEAT_CAP 256
typedef struct {
    u32 uid;
    float bright;
    int on;
    H node;
    int has_node;
} HeatEnt;
static HeatEnt g_heat[HEAT_CAP];
static DWORD g_heat_last_ms;

static float heat_now_sec(void) {
    return (float)GetTickCount() * 0.001f;
}

static void heat_decay_all(void) {
    DWORD now = GetTickCount();
    float dt;
    if (!g_heat_last_ms) { g_heat_last_ms = now; return; }
    dt = (float)(now - g_heat_last_ms) * 0.001f;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.25f) dt = 0.25f; /* clamp large stalls */
    g_heat_last_ms = now;
    if (dt <= 0.0f) return;
    /* half-life ~0.45s => per-second multiply ~0.22 */
    {
        float k = 1.0f;
        /* approx exp(-1.5 * dt) */
        float x = 3.0f * dt; /* half fade time */
        k = 1.0f - x + 0.5f * x * x; /* cheap exp approx for small x */
        if (k < 0.05f) k = 0.05f;
        if (dt > 0.05f) {
            /* better for larger steps */
            int n = (int)(dt * 60.0f);
            if (n < 1) n = 1;
            if (n > 30) n = 30;
            k = 1.0f;
            for (int i = 0; i < n; i++) k *= 0.95f; /* ~2x faster fade */
        }
        for (int i = 0; i < HEAT_CAP; i++) {
            if (!g_heat[i].on) continue;
            g_heat[i].bright *= k;
            if (g_heat[i].bright < 0.02f) {
                g_heat[i].on = 0;
                g_heat[i].bright = 0.0f;
                g_heat[i].has_node = 0;
            }
        }
    }
}

__declspec(dllexport) void cvm_heat_pulse(u32 uid, const H node_key) {
    int free_i = -1;
    int oldest = 0;
    float oldest_b = 1e9f;
    heat_decay_all();
    if (uid == 0) return;
    for (int i = 0; i < HEAT_CAP; i++) {
        if (g_heat[i].on && g_heat[i].uid == uid) {
            g_heat[i].bright += 0.55f;
            if (g_heat[i].bright > 1.0f) g_heat[i].bright = 1.0f;
            if (node_key) {
                memcpy(g_heat[i].node, node_key, 32);
                g_heat[i].has_node = 1;
            }
            return;
        }
        if (!g_heat[i].on && free_i < 0) free_i = i;
        if (g_heat[i].on && g_heat[i].bright < oldest_b) {
            oldest_b = g_heat[i].bright;
            oldest = i;
        }
    }
    {
        int i = free_i >= 0 ? free_i : oldest;
        memset(&g_heat[i], 0, sizeof(g_heat[i]));
        g_heat[i].on = 1;
        g_heat[i].uid = uid;
        g_heat[i].bright = 0.55f; /* single hit still visible */
        if (node_key) {
            memcpy(g_heat[i].node, node_key, 32);
            g_heat[i].has_node = 1;
        }
    }
}

__declspec(dllexport) float cvm_heat_uid(u32 uid) {
    heat_decay_all();
    if (!uid) return 0.0f;
    for (int i = 0; i < HEAT_CAP; i++)
        if (g_heat[i].on && g_heat[i].uid == uid) return g_heat[i].bright;
    return 0.0f;
}

__declspec(dllexport) float cvm_heat_node(const H key) {
    float m = 0.0f;
    heat_decay_all();
    if (!key) return 0.0f;
    for (int i = 0; i < HEAT_CAP; i++) {
        if (!g_heat[i].on || !g_heat[i].has_node) continue;
        if (same(g_heat[i].node, key) && g_heat[i].bright > m) m = g_heat[i].bright;
    }
    return m;
}

__declspec(dllexport) void cvm_heat_tick(void) {
    heat_decay_all();
}
