#ifndef CONTINUE_H
#define CONTINUE_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

#define CACHE_CAP 256

typedef struct { H tok; u8 *dat; u32 len; } CE;

static SOCKET  _s   = INVALID_SOCKET;
static H       _me;
static int     _inited;
static CE      _c[CACHE_CAP];
static u32     _ci;


static const H ZERO = {0};

static void _init(void) {
    if (_inited) return;
    _inited = 1;

    FILE *f = fopen("id.bin", "rb");
    if (f) {
        fread(_me, 1, 32, f);
        fclose(f);
    } else {
        memset(_me, 0, 32);
    }

    WSADATA w;
    WSAStartup(MAKEWORD(2,2), &w);
    _s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    connect(_s, (void*)&a, sizeof(a));
}

static void _send(u8 op, void *body, u32 len) {
    u8 h[5] = {op, len>>24, len>>16, len>>8, len};
    send(_s, (char*)h, 5, 0);
    if (len) send(_s, (char*)body, len, 0);
}

static u8* _recv(u32 *out_len) {
    u8 h[5];
    u32 g = 0;
    while (g < 5) { int r = recv(_s, (char*)h+g, 5-g, 0); if (r < 1) return 0; g += r; }
    u32 l = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4];
    if (out_len) *out_len = l;
    if (l == 0) return 0;
    u8 *b = malloc(l+1);
    g = 0;
    while (g < l) { int r = recv(_s, (char*)b+g, l-g, 0); if (r < 1) { free(b); return 0; } g += r; }
    return b;
}

static u8* _file(H h) { _send(3, h, 32); return _recv(0); }
static u8* _uget(H key) { u8 b[64]; memcpy(b, _me, 32); memcpy(b+32, key, 32); _send(8, b, 64); return _recv(0); }
static void _uset(H key, H val) { u8 b[96]; memcpy(b, _me, 32); memcpy(b+32, key, 32); memcpy(b+64, val, 32); _send(7, b, 96); u8 *r = _recv(0); free(r); }

static u32 r32le(u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static u32 _span(u8 *blk) { return r32le(blk + 32); }

static u8* _cget(H tok, u32 *len) {
    for (u32 i = 0; i < CACHE_CAP; i++)
        if (_c[i].dat && memcmp(_c[i].tok, tok, 32) == 0) {
            *len = _c[i].len;
            return _c[i].dat;
        }
    return 0;
}

static void _cput(H tok, u8 *dat, u32 len) {
    free(_c[_ci].dat);
    memcpy(_c[_ci].tok, tok, 32);
    _c[_ci].dat = malloc(len);
    memcpy(_c[_ci].dat, dat, len);
    _c[_ci].len = len;
    _ci = (_ci + 1) % CACHE_CAP;
}


static void* _find(H tok) {
    char path[64] = "mods/";
    for (int i = 0; i < 32; i++) sprintf(path+5+i*2, "%02x", tok[i]);
    strcat(path, ".dll");
    HMODULE m = LoadLibraryA(path);
    return m ? (void*)GetProcAddress(m, "run") : 0;
}

static u8* _resolve(H tok, u32 *out_len) {
    u32 clen;
    u8 *c = _cget(tok, &clen);
    if (c) {
        *out_len = clen;
        u8 *r = malloc(clen);
        memcpy(r, c, clen);
        return r;
    }

    u8 *v = _uget(tok);
    if (v) {
        H ch;
        memcpy(ch, v, 32);
        free(v);
        u8 *blk = _file(ch);
        if (blk) {
            u32 blen = 32 + _span(blk);
            _cput(tok, blk, blen);
            *out_len = blen;
            return blk;
        }
    }

    u8 *blk = _file(tok);
    if (blk) {
        u32 blen = 32 + _span(blk);
        _cput(tok, blk, blen);
        *out_len = blen;
        return blk;
    }

    return 0;
}

static int cont(u8 **pp) {
    _init();
    u8 *p = *pp;

    H tok;
    memcpy(tok, p, 32);
    if (memcmp(tok, ZERO, 32) == 0) return 0;

    u32 sp = _span(p);
    u32 pl = sp - 4;
    u8 *ld = p + 36;
    p += 32 + sp;
    *pp = p;

    void *fn = _find(tok);
    if (fn) {
        ((void(*)())fn)();
        goto next;
    }

    u32 rlen;
    u8 *rd = _resolve(tok, &rlen);
    if (rd) {
        u8 *rp = rd;
        cont(&rp);
        free(rd);
    }

    next:;
    return 1;
}

#endif
