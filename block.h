#ifndef BLOCK_H
#define BLOCK_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")

#ifndef CVM_TYPES_DEFINED
#define CVM_TYPES_DEFINED
typedef unsigned char u8;
typedef unsigned u32;
typedef unsigned long long u64;
typedef u8 H[32];
#endif

static int sha256(const u8 *data, u32 len, H out) {
    HCRYPTPROV hp = 0;
    HCRYPTHASH hh = 0;
    DWORD sz = 32;
    BOOL ok = 0;
    if (CryptAcquireContextW(&hp, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hp, CALG_SHA_256, 0, 0, &hh)) {
            if (CryptHashData(hh, data, len, 0))
                ok = CryptGetHashParam(hh, HP_HASHVAL, out, &sz, 0);
            CryptDestroyHash(hh);
        }
        CryptReleaseContext(hp, 0);
    }
    return ok;
}

static SOCKET _bs = INVALID_SOCKET;
static int    _bn;

static void _bnet(void) {
    if (_bn) return;
    _bn = 1;
    WSADATA w;
    WSAStartup(MAKEWORD(2,2), &w);
    _bs = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port   = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    connect(_bs, (void*)&a, sizeof(a));
}

static void _bhex(const H h, char *out) {
    for (int i = 0; i < 32; i++) sprintf(out + i*2, "%02x", h[i]);
    out[64] = 0;
}

static u8* _cload(const H h, u32 *out_len) {
    char hex[65]; _bhex(h, hex);
    char path[75] = "cache/"; strcat(path, hex);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    u32 len = (u32)ftell(f);
    fseek(f, 0, SEEK_SET);
    u8 *d = malloc(len + 1);
    fread(d, 1, len, f);
    fclose(f);
    if (out_len) *out_len = len;
    return d;
}

static void _cstore(const H h, const u8 *d, u32 len) {
    char hex[65]; _bhex(h, hex);
    char path[75] = "cache/"; strcat(path, hex);
    CreateDirectoryA("cache", NULL);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite(d, 1, len, f);
    fclose(f);
}

static u8* _bfetch(const H h, u32 *out_len) {
    _bnet();
    u8 fh[5] = {3, 0,0,0,32};
    send(_bs, (char*)fh, 5, 0);
    send(_bs, (char*)h, 32, 0);
    u8 rh[5]; u32 g = 0;
    while (g < 5) { int r = recv(_bs, (char*)rh+g, 5-g, 0); if (r < 1) return 0; g += r; }
    u32 l = (u32)rh[1]<<24 | rh[2]<<16 | rh[3]<<8 | rh[4];
    if (rh[0] != 0) { while (l > 0) { char x; recv(_bs, &x, 1, 0); l--; } return 0; }
    if (l == 0) return 0;
    u8 *d = malloc(l + 1);
    g = 0;
    while (g < l) { int r = recv(_bs, (char*)d+g, l-g, 0); if (r < 1) { free(d); return 0; } g += r; }
    if (out_len) *out_len = l;
    return d;
}

#define UQ_CAP 128
static struct {
    H    q[UQ_CAP];
    u32  hd, tl;
    CRITICAL_SECTION cs;
    HANDLE th, ev;
    volatile int run;
} _uq;

static DWORD WINAPI _uq_thread(void *p) {
    (void)p;
    while (_uq.run) {
        WaitForSingleObject(_uq.ev, 5000);
        if (!_uq.run) break;
        H batch[UQ_CAP]; u32 n = 0;
        EnterCriticalSection(&_uq.cs);
        while (_uq.hd != _uq.tl && n < UQ_CAP) {
            memcpy(batch[n], _uq.q[_uq.hd], 32);
            _uq.hd = (_uq.hd + 1) % UQ_CAP; n++;
        }
        LeaveCriticalSection(&_uq.cs);
        for (u32 i = 0; i < n; i++) {
            u32 len; u8 *d = _cload(batch[i], &len);
            if (d) {
                SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in a = {0};
                a.sin_family = AF_INET; a.sin_port = htons(9000);
                inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
                if (connect(s, (void*)&a, sizeof(a)) == 0) {
                    u8 h[5] = {2, len>>24, len>>16, len>>8, len};
                    send(s, (char*)h, 5, 0); send(s, (char*)d, len, 0);
                    u8 rh[5]; u32 g = 0;
                    while (g < 5) { int r = recv(s, (char*)rh+g, 5-g, 0); if (r < 1) break; g += r; }
                    if (g == 5) {
                        u32 rl = (u32)rh[1]<<24 | rh[2]<<16 | rh[3]<<8 | rh[4];
                        while (rl > 0) { char x; recv(s, &x, 1, 0); rl--; }
                    }
                }
                closesocket(s); free(d);
            }
            Sleep(500);
        }
    }
    return 0;
}

static void _uq_start(void) {
    if (_uq.run) return;
    _uq.hd = _uq.tl = 0;
    InitializeCriticalSection(&_uq.cs);
    _uq.ev = CreateEvent(NULL, FALSE, FALSE, NULL);
    _uq.run = 1;
    _uq.th = CreateThread(NULL, 0, _uq_thread, NULL, 0, NULL);
}

static void _uq_put(const H h) {
    EnterCriticalSection(&_uq.cs);
    u32 nx = (_uq.tl + 1) % UQ_CAP;
    if (nx != _uq.hd) { memcpy(_uq.q[_uq.tl], h, 32); _uq.tl = nx; }
    LeaveCriticalSection(&_uq.cs);
    SetEvent(_uq.ev);
}

static u8* _last_data;
static u32 _last_len;
static H   _last_hash;
static H  *_last_ph;

static u8* block(H *ph, u32 *out_len) {
    if (_last_data) {
        H nh;
        sha256(_last_data, _last_len, nh);
        if (memcmp(nh, _last_hash, 32) != 0) {
            _cstore(nh, _last_data, _last_len);
            _uq_start();
            _uq_put(nh);
            if (_last_ph) memcpy(*_last_ph, nh, 32);
        }
        free(_last_data);
        _last_data = 0;
    }

    u32 len;
    u8 *d = _cload(*ph, &len);
    if (!d) {
        d = _bfetch(*ph, &len);
        if (d) {
            _cstore(*ph, d, len);
        }
    }

    if (d) {
        _last_data = malloc(len + 1);
        memcpy(_last_data, d, len);
        _last_data[len] = 0;
        _last_len = len;
        memcpy(_last_hash, *ph, 32);
        _last_ph = ph;
        free(d);
        if (out_len) *out_len = len;
        return _last_data;
    }

    return 0;
}

#endif
