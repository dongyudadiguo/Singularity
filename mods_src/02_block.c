// 02_block: block editing operations (count, hash, data, item, end, ins, del, set)

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define H 32

typedef unsigned char u8;

typedef struct {
    u8 *p;
    DWORD n;
} Buf;

typedef struct {
    Buf f;
    DWORD off;
    u8 key[H];
} Frame;

typedef void (*Op)(u8 *data, uint32_t len);

typedef struct Host {
    void (*op)(u8 *id, Op fn);
    void (*op_name)(char *name, Op fn);
    void (*del)(u8 *id);
    void (*del_name)(char *name);

    void (*override)(u8 *key, u8 *file, DWORD len);
    void (*touch)();

    Buf  (*rpc)(uint8_t op, u8 *body, DWORD len);

    void (*run)(u8 *hash);
    void (*enter)(u8 *hash);
    void (*adv)();

    void (*push)(u8 *p, DWORD n);
    Buf  (*pop)();
    Buf *(*top)();

    Frame *cur;
} Host;

static Host *h;

static uint32_t u32(u8 *p) {
    uint32_t x = 0;
    memcpy(&x, p, 4);
    return x;
}

static void put32(u8 *p, uint32_t x) {
    memcpy(p, &x, 4);
}

static int z32(u8 *p) {
    for (int i = 0; i < 32; i++)
        if (p[i])
            return 0;
    return 1;
}

static uint32_t pop32() {
    Buf b = h->pop();
    uint32_t x = b.n >= 4 ? u32(b.p) : 0;
    free(b.p);
    return x;
}

static void push32(uint32_t x) {
    u8 b[4];
    put32(b, x);
    h->push(b, 4);
}

static uint32_t count_items(Buf *b) {
    DWORD o = 0;
    uint32_t c = 0;

    while (o + 32 <= b->n && !z32(b->p + o)) {
        if (o + 36 > b->n)
            break;

        uint32_t span = u32(b->p + o + 32);

        if (span < 4 || o + 32 + span > b->n)
            break;

        c++;
        o += 32 + span;
    }

    return c;
}

static int itempos(Buf *b, uint32_t idx, DWORD *po, DWORD *pn) {
    DWORD o = 0;
    uint32_t c = 0;

    while (o + 32 <= b->n && !z32(b->p + o)) {
        if (o + 36 > b->n)
            break;

        uint32_t span = u32(b->p + o + 32);

        if (span < 4 || o + 32 + span > b->n)
            break;

        if (c == idx) {
            *po = o;
            *pn = 32 + span;
            return 1;
        }

        c++;
        o += 32 + span;
    }

    return 0;
}

static DWORD insertpos(Buf *b, uint32_t idx) {
    DWORD o = 0;
    uint32_t c = 0;

    while (o + 32 <= b->n && !z32(b->p + o)) {
        if (c >= idx)
            return o;

        if (o + 36 > b->n)
            return b->n;

        uint32_t span = u32(b->p + o + 32);

        if (span < 4 || o + 32 + span > b->n)
            return b->n;

        c++;
        o += 32 + span;
    }

    return o;
}

static int has_end(Buf *b) {
    DWORD o = 0;

    while (o + 32 <= b->n) {
        if (z32(b->p + o))
            return 1;

        if (o + 36 > b->n)
            return 0;

        uint32_t span = u32(b->p + o + 32);

        if (span < 4 || o + 32 + span > b->n)
            return 0;

        o += 32 + span;
    }

    return 0;
}

static void op_count(u8 *d, uint32_t n) {
    Buf b = h->pop();
    push32(count_items(&b));
    free(b.p);
    h->adv();
}

static void op_hash(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf b = h->pop();
    DWORD o, sz;
    u8 z[32] = {0};

    if (itempos(&b, idx, &o, &sz))
        h->push(b.p + o, 32);
    else
        h->push(z, 32);

    free(b.p);
    h->adv();
}

static void op_data(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf b = h->pop();
    DWORD o, sz;

    if (itempos(&b, idx, &o, &sz)) {
        uint32_t span = u32(b.p + o + 32);
        h->push(b.p + o + 36, span - 4);
    } else {
        h->push(0, 0);
    }

    free(b.p);
    h->adv();
}

static void op_item(u8 *d, uint32_t n) {
    Buf payload = h->pop();
    Buf tok = h->pop();

    DWORD outn = 36 + payload.n;
    uint32_t span = 4 + payload.n;
    u8 *o = malloc(outn);

    memset(o, 0, 32);
    if (tok.n) memcpy(o, tok.p, tok.n > 32 ? 32 : tok.n);
    put32(o + 32, span);
    if (payload.n) memcpy(o + 36, payload.p, payload.n);

    h->push(o, outn);

    free(o);
    free(tok.p);
    free(payload.p);

    h->adv();
}

static void op_end(u8 *d, uint32_t n) {
    u8 z[32] = {0};
    h->push(z, 32);
    h->adv();
}

static void op_ins(u8 *d, uint32_t n) {
    Buf item = h->pop();
    uint32_t idx = pop32();
    Buf file = h->pop();

    DWORD pos = insertpos(&file, idx);
    DWORD add = has_end(&file) ? 0 : 32;
    DWORD outn = file.n + item.n + add;
    u8 *o = malloc(outn);
    DWORD k = 0;

    if (pos) memcpy(o + k, file.p, pos);
    k += pos;

    if (item.n) memcpy(o + k, item.p, item.n);
    k += item.n;

    if (file.n > pos) memcpy(o + k, file.p + pos, file.n - pos);
    k += file.n - pos;

    if (add) {
        memset(o + k, 0, 32);
        k += 32;
    }

    h->push(o, outn);

    free(o);
    free(file.p);
    free(item.p);

    h->adv();
}

static void op_del(u8 *d, uint32_t n) {
    uint32_t idx = pop32();
    Buf file = h->pop();
    DWORD pos, sz;

    if (!itempos(&file, idx, &pos, &sz)) {
        h->push(file.p, file.n);
        free(file.p);
        h->adv();
        return;
    }

    DWORD outn = file.n - sz;
    u8 *o = malloc(outn);

    if (pos) memcpy(o, file.p, pos);
    if (file.n > pos + sz) memcpy(o + pos, file.p + pos + sz, file.n - pos - sz);

    h->push(o, outn);

    free(o);
    free(file.p);

    h->adv();
}

static void op_set(u8 *d, uint32_t n) {
    Buf item = h->pop();
    uint32_t idx = pop32();
    Buf file = h->pop();
    DWORD pos, sz;

    if (!itempos(&file, idx, &pos, &sz)) {
        h->push(file.p, file.n);
        free(file.p);
        free(item.p);
        h->adv();
        return;
    }

    DWORD outn = file.n - sz + item.n;
    u8 *o = malloc(outn);
    DWORD k = 0;

    if (pos) memcpy(o + k, file.p, pos);
    k += pos;

    if (item.n) memcpy(o + k, item.p, item.n);
    k += item.n;

    if (file.n > pos + sz) memcpy(o + k, file.p + pos + sz, file.n - pos - sz);

    h->push(o, outn);

    free(o);
    free(file.p);
    free(item.p);

    h->adv();
}

__declspec(dllexport)
void cvm_init(Host *host) {
    h = host;

    host->op_name("BLK:COUNT", op_count);
    host->op_name("BLK:HASH", op_hash);
    host->op_name("BLK:DATA", op_data);
    host->op_name("BLK:ITEM", op_item);
    host->op_name("BLK:END", op_end);
    host->op_name("BLK:INS", op_ins);
    host->op_name("BLK:DEL", op_del);
    host->op_name("BLK:SET", op_set);
}