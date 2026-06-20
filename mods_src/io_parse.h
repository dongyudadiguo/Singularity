#ifndef MOD_IO_PARSE_H
#define MOD_IO_PARSE_H

#include "../cvm_state.h"
#include "../continue.h"
#include "../block.h"
#include <ctype.h>

static int hex_val(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_hex_hash_bytes(u8 *d, u32 len, H out) {
    cvm_zero(out);
    if (!d || len != 64) return 0;
    for (int i = 0; i < 32; i++) {
        int hi = hex_val(d[i * 2]);
        int lo = hex_val(d[i * 2 + 1]);
        if (hi < 0 || lo < 0) return 0;
        out[i] = (u8)((hi << 4) | lo);
    }
    return 1;
}

static int parse_u64_bytes(u8 *d, u32 len, u64 *out) {
    u64 v = 0;
    if (!d || !len) return 0;
    for (u32 i = 0; i < len; i++) {
        if (d[i] < '0' || d[i] > '9') return 0;
        v = v * 10 + (u64)(d[i] - '0');
    }
    *out = v;
    return 1;
}

static void print_h(H h) {
    for (int i = 0; i < 32; i++) printf("%02x", h[i]);
}

#endif
