#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define H 32
typedef unsigned char u8;

typedef struct { u8 *p; DWORD n; } Buf;
typedef struct { Buf f; DWORD off; u8 key[H]; } Frame;
typedef int (*Op)(u8 *data, uint32_t len);

typedef struct Host {
    void (*op)(u8 *id, Op fn);
    void (*op_name)(char *name, Op fn);
    void (*del)(u8 *id);
    void (*del_name)(char *name);

    void (*override)(u8 *key, u8 *file, DWORD len);
    void (*touch)();

    Buf  (*post)(wchar_t *path, u8 *body, DWORD len);

    void (*run)(u8 *hash);
    void (*enter)(u8 *hash);
    void (*adv)();

    void (*push)(u8 *p, DWORD n);
    Buf  (*pop)();
    Buf *(*top)();

    Frame *cur;
} Host;

static uint32_t rd32(u8 *p) { return *(uint32_t *)p; }
static void wr32(u8 *p, uint32_t x) { *(uint32_t *)p = x; }
static uint64_t rd64(u8 *p) { return *(uint64_t *)p; }
static void wr64(u8 *p, uint64_t x) { *(uint64_t *)p = x; }

static Buf mbuf(DWORD n) {
    Buf b = { malloc(n), n };
    return b;
}
