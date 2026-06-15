// 04_data — 列表、键常量
// gcc -shared mods_src/04_data.c -Os -s -o mods/04_data.dll

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define H 32

typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;

typedef struct {
    void (*op)(u8 *, void (*)(u8 *, uint32_t));
    void (*op_name)(char *, void (*)(u8 *, uint32_t));
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
} Host;

static Host *h;

static uint32_t U(u8 *p) { uint32_t x; memcpy(&x, p, 4); return x; }
static void WU(u8 *p, uint32_t v) { memcpy(p, &v, 4); }
static void push_u32(uint32_t v) { u8 buf[4]; WU(buf, v); h->push(buf, 4); }

// List implementation: stored as contiguous blocks
// Each list item: [32 byte key][4 byte len][data...]
#define MAX_LISTS 256
#define LIST_MAX_ITEMS 4096

static struct {
    u8 name[H];
    Buf items[LIST_MAX_ITEMS];
    int count;
} lists[MAX_LISTS];
static int listn = 0;

static int list_find(u8 *name) {
    for (int i = 0; i < listn; i++) {
        if (!memcmp(lists[i].name, name, H)) return i;
    }
    return -1;
}

// LST: list operations
static void lst_new(u8 *p, uint32_t n) {
    // stack: list_name
    Buf name = h->pop();
    if (listn >= MAX_LISTS) return;
    
    int idx = list_find(name.p);
    if (idx >= 0) return; // already exists
    
    idx = listn++;
    memcpy(lists[idx].name, name.p, name.n < H ? name.n : H);
    if (name.n < H) memset(lists[idx].name + name.n, 0, H - name.n);
    lists[idx].count = 0;
}

static void lst_count(u8 *p, uint32_t n) {
    // stack: list_name
    Buf name = h->pop();
    int idx = list_find(name.p);
    push_u32(idx >= 0 ? lists[idx].count : 0);
}

static void lst_get(u8 *p, uint32_t n) {
    // stack: list_name, index
    uint32_t idx = pop_u32();
    Buf name = h->pop();
    int li = list_find(name.p);
    if (li >= 0 && (int)idx < lists[li].count) {
        h->push(lists[li].items[idx].p, lists[li].items[idx].n);
    } else {
        h->push(0, 0);
    }
}

static void lst_push(u8 *p, uint32_t n) {
    // stack: list_name, item_data
    Buf item = h->pop();
    Buf name = h->pop();
    int li = list_find(name.p);
    if (li >= 0 && lists[li].count < LIST_MAX_ITEMS) {
        int ci = lists[li].count++;
        lists[li].items[ci].p = malloc(item.n);
        if (lists[li].items[ci].p) {
            memcpy(lists[li].items[ci].p, item.p, item.n);
            lists[li].items[ci].n = item.n;
        }
    }
}

static void lst_del(u8 *p, uint32_t n) {
    // stack: list_name, index
    uint32_t idx = pop_u32();
    Buf name = h->pop();
    int li = list_find(name.p);
    if (li >= 0 && (int)idx < lists[li].count) {
        free(lists[li].items[idx].p);
        for (int i = idx; i < lists[li].count - 1; i++) {
            lists[li].items[i] = lists[li].items[i + 1];
        }
        lists[li].count--;
    }
}

static void lst_join(u8 *p, uint32_t n) {
    // stack: list_name, separator
    Buf sep = h->pop();
    Buf name = h->pop();
    int li = list_find(name.p);
    
    if (li < 0 || lists[li].count == 0) {
        h->push(0, 0);
        return;
    }
    
    // Calculate total size
    uint32_t total = 0;
    for (int i = 0; i < lists[li].count; i++) {
        total += lists[li].items[i].n;
    }
    total += sep.n * (lists[li].count - 1);
    
    u8 *buf = malloc(total);
    uint32_t off = 0;
    for (int i = 0; i < lists[li].count; i++) {
        if (i > 0 && sep.n) {
            memcpy(buf + off, sep.p, sep.n);
            off += sep.n;
        }
        if (lists[li].items[i].n) {
            memcpy(buf + off, lists[li].items[i].p, lists[li].items[i].n);
            off += lists[li].items[i].n;
        }
    }
    h->push(buf, total);
    free(buf);
}

// KEY: key constants
static void key_esc(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x1B;h->push(k,H); }
static void key_enter(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x0D;h->push(k,H); }
static void key_back(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x08;h->push(k,H); }
static void key_del(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x7F;h->push(k,H); }
static void key_tab(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x09;h->push(k,H); }
static void key_space(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x20;h->push(k,H); }
static void key_left(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x25;h->push(k,H); }
static void key_right(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x27;h->push(k,H); }
static void key_up(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x26;h->push(k,H); }
static void key_down(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x28;h->push(k,H); }
static void key_home(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x24;h->push(k,H); }
static void key_end(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x23;h->push(k,H); }
static void key_pgup(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x21;h->push(k,H); }
static void key_pgdn(u8 *p, uint32_t n) { u8 k[H]={0};k[0]=0x22;h->push(k,H); }

static void key_code(u8 *p, uint32_t n) {
    // Push the opcode as a key
    if (n >= 1) {
        u8 k[H] = {0};
        k[0] = p[0];
        h->push(k, H);
    }
}

static void key_ascii(u8 *p, uint32_t n) {
    // Push ASCII value as key
    if (n >= 1) {
        u8 k[H] = {0};
        k[0] = p[0];
        h->push(k, H);
    }
}

static void key_mods(u8 *p, uint32_t n) {
    // Push modifier flags
    u8 k[H] = {0};
    h->push(k, H);
}

void cvm_init(Host *host) {
    h = host;
    h->op_name("LST:NEW", lst_new);
    h->op_name("LST:COUNT", lst_count);
    h->op_name("LST:GET", lst_get);
    h->op_name("LST:PUSH", lst_push);
    h->op_name("LST:DEL", lst_del);
    h->op_name("LST:JOIN", lst_join);
    h->op_name("KEY:ESC", key_esc);
    h->op_name("KEY:ENTER", key_enter);
    h->op_name("KEY:BACK", key_back);
    h->op_name("KEY:DEL", key_del);
    h->op_name("KEY:TAB", key_tab);
    h->op_name("KEY:SPACE", key_space);
    h->op_name("KEY:LEFT", key_left);
    h->op_name("KEY:RIGHT", key_right);
    h->op_name("KEY:UP", key_up);
    h->op_name("KEY:DOWN", key_down);
    h->op_name("KEY:HOME", key_home);
    h->op_name("KEY:END", key_end);
    h->op_name("KEY:PGUP", key_pgup);
    h->op_name("KEY:PGDN", key_pgdn);
    h->op_name("KEY:CODE", key_code);
    h->op_name("KEY:ASCII", key_ascii);
    h->op_name("KEY:MODS", key_mods);
}