#ifndef MOD_BLOCK_CHAIN_H
#define MOD_BLOCK_CHAIN_H

#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"

static u32 bc_span(u8 *p) {
    return (u32)p[32] | ((u32)p[33] << 8) | ((u32)p[34] << 16) | ((u32)p[35] << 24);
}

static int bc_end(u8 *p, u32 len, u32 off) {
    if (off + 32 > len) return 1;
    for (int i = 0; i < 32; i++) if (p[off + i]) return 0;
    return 1;
}

static int bc_range(u8 *p, u32 len, u32 idx, u32 *start, u32 *end) {
    u32 off = 0;
    u32 cur = 0;
    while (!bc_end(p, len, off)) {
        if (off + 36 > len) return 0;
        u32 sp = bc_span(p + off);
        if (sp < 4 || off + 32 + sp > len) return 0;
        if (cur == idx) {
            *start = off;
            *end = off + 32 + sp;
            return 1;
        }
        off += 32 + sp;
        cur++;
    }
    return 0;
}

static u32 bc_count(u8 *p, u32 len) {
    u32 off = 0;
    u32 n = 0;
    while (!bc_end(p, len, off)) {
        if (off + 36 > len) break;
        u32 sp = bc_span(p + off);
        if (sp < 4 || off + 32 + sp > len) break;
        off += 32 + sp;
        n++;
    }
    return n;
}

static int bc_save_edit(u8 *a, u32 alen, u8 *b, u32 blen, u8 *c, u32 clen, H out) {
    u32 total = alen + blen + clen;
    u8 *buf = malloc(total + 1);
    if (!buf) { cvm_zero(out); return 0; }
    if (a && alen) memcpy(buf, a, alen);
    if (b && blen) memcpy(buf + alen, b, blen);
    if (c && clen) memcpy(buf + alen + blen, c, clen);
    block_write(buf, total, out);
    free(buf);
    return 1;
}

#endif
