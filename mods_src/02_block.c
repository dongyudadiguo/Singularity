// 02_block — 块编辑（插入、删除、修改）
// gcc -shared mods_src/02_block.c -Os -s -o mods/02_block.dll

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define H 32

typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;

typedef struct {
    void (*op)(u8 *, void (*)(void));
    void (*op_name)(char *, void (*)(void));
    void (*del)(u8 *);
    void (*del_name)(char *);
    void (*override)(u8 *, u8 *, uint32_t);
    void (*touch)(void);
    Buf (*rpc)(uint8_t, u8 *, uint32_t);
    void (*run)(u8 *);
    void (*enter)(u8 *);
    void (*adv)(void);
    void (*push)(u8 *, uint32_t);
    Buf (*pop)(void);
    Buf *(*top)(void);
    void *cur;
    u8 *pay; uint32_t plen;
    void (*next)(void);
    void (*next_noadv)(void);
} Host;

static Host *h;

static uint32_t U(u8 *p) { uint32_t x; memcpy(&x, p, 4); return x; }
static void WU(u8 *p, uint32_t v) { memcpy(p, &v, 4); }

static uint32_t pop_u32(void) { Buf b=h->pop(); return b.n>=4?U(b.p):0; }
static void push_u32(uint32_t v) { u8 buf[4];WU(buf,v); h->push(buf,4); }

// Block format: [32 byte token][4 byte span (u32le)][payload...]
// End marker: 32 zero bytes

static int is_zero(u8 *p, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) if (p[i]) return 0;
    return 1;
}

static uint32_t blk_count(u8 *data, uint32_t len) {
    uint32_t cnt = 0;
    uint32_t off = 0;
    while (off + H <= len && !is_zero(data + off, H)) {
        uint32_t span = off + H <= len ? U(data + off + H) : 0;
        off += H + span;
        cnt++;
    }
    return cnt;
}

// BLK: block operations
static void blk_count_op(void) {
    Buf b = h->pop();
    push_u32(blk_count(b.p, b.n));
    h->next();
}

static void blk_hash(void) {
    uint32_t idx = pop_u32();
    Buf b = h->pop();
    uint32_t cnt = 0;
    uint32_t off = 0;
    u8 z[H]; memset(z,0,H);
    while (off + H <= b.n && !is_zero(b.p + off, H)) {
        if (cnt == idx) { h->push(b.p + off, H); h->next(); return; }
        uint32_t span = U(b.p + off + H);
        off += H + span;
        cnt++;
    }
    h->push(z, H);
    h->next();
}

static void blk_data(void) {
    uint32_t idx = pop_u32();
    Buf b = h->pop();
    uint32_t cnt = 0;
    uint32_t off = 0;
    while (off + H <= b.n && !is_zero(b.p + off, H)) {
        if (cnt == idx) {
            uint32_t span = U(b.p + off + H);
            h->push(b.p + off + H + 4, span);
            h->next();
            return;
        }
        uint32_t span = U(b.p + off + H);
        off += H + span;
        cnt++;
    }
    h->push(0, 0);
    h->next();
}

static void blk_item(void) {
    uint32_t idx = pop_u32();
    Buf b = h->pop();
    uint32_t cnt = 0;
    uint32_t off = 0;
    while (off + H <= b.n && !is_zero(b.p + off, H)) {
        if (cnt == idx) {
            uint32_t span = U(b.p + off + H);
            h->push(b.p + off, H + 4 + span);
            h->next();
            return;
        }
        uint32_t span = U(b.p + off + H);
        off += H + span;
        cnt++;
    }
    h->push(0, 0);
    h->next();
}

static void blk_end(void) {
    Buf b = h->pop();
    uint32_t off = 0;
    while (off + H <= b.n && !is_zero(b.p + off, H)) {
        uint32_t span = U(b.p + off + H);
        off += H + span;
    }
    push_u32(off);
    h->next();
}

static void blk_ins(void) {
    uint32_t at = pop_u32();
    Buf data = h->pop();
    Buf b = h->pop();
    
    uint32_t item_len = H + 4 + data.n;
    u8 *item = malloc(item_len);
    memcpy(item, data.p, data.n < H ? data.n : H);
    memset(item + (data.n < H ? data.n : H), 0, H - (data.n < H ? data.n : H));
    WU(item + H, data.n);
    memcpy(item + H + 4, data.p, data.n);
    
    if (at > b.n) at = b.n;
    uint32_t new_len = b.n + item_len;
    u8 *new_data = malloc(new_len);
    memcpy(new_data, b.p, at);
    memcpy(new_data + at, item, item_len);
    memcpy(new_data + at + item_len, b.p + at, b.n - at);
    h->push(new_data, new_len);
    free(item);
    free(new_data);
    h->next();
}

static void blk_del(void) {
    uint32_t at = pop_u32();
    Buf b = h->pop();
    
    if (at >= b.n) { h->push(b.p, b.n); h->next(); return; }
    
    uint32_t span = at + H <= b.n ? U(b.p + at + H) : 0;
    uint32_t item_len = H + 4 + span;
    
    uint32_t new_len = b.n - item_len;
    u8 *new_data = malloc(new_len);
    memcpy(new_data, b.p, at);
    memcpy(new_data + at, b.p + at + item_len, b.n - at - item_len);
    h->push(new_data, new_len);
    free(new_data);
    h->next();
}

static void blk_set(void) {
    Buf old_hash = h->pop();
    Buf new_data = h->pop();
    Buf b = h->pop();
    
    uint32_t off = 0;
    while (off + H <= b.n && !is_zero(b.p + off, H)) {
        if (!memcmp(b.p + off, old_hash.p, H)) {
            uint32_t old_span = U(b.p + off + H);
            uint32_t before = off;
            uint32_t after_start = off + H + 4 + old_span;
            uint32_t after_len = b.n - after_start;
            
            uint32_t new_len = before + H + 4 + new_data.n + after_len;
            u8 *new_data_buf = malloc(new_len);
            memcpy(new_data_buf, b.p, before);
            memcpy(new_data_buf + before, b.p + off, H);
            WU(new_data_buf + before + H, new_data.n);
            memcpy(new_data_buf + before + H + 4, new_data.p, new_data.n);
            memcpy(new_data_buf + before + H + 4 + new_data.n, b.p + after_start, after_len);
            h->push(new_data_buf, new_len);
            free(new_data_buf);
            h->next();
            return;
        }
        uint32_t span = U(b.p + off + H);
        off += H + 4 + span;
    }
    h->push(b.p, b.n);
    h->next();
}

void cvm_init(Host *host) {
    h = host;
    h->op_name("BLK:COUNT", blk_count_op);
    h->op_name("BLK:HASH", blk_hash);
    h->op_name("BLK:DATA", blk_data);
    h->op_name("BLK:ITEM", blk_item);
    h->op_name("BLK:END", blk_end);
    h->op_name("BLK:INS", blk_ins);
    h->op_name("BLK:DEL", blk_del);
    h->op_name("BLK:SET", blk_set);
}