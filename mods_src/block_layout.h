#ifndef BLOCK_LAYOUT_H
#define BLOCK_LAYOUT_H
#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;

/* Instruction: token_len[u32] + token[token_len] + payload_len[u32] + payload[payload_len]
 * End marker: token_len == 0 (4 bytes).
 */
static u32 bl_tlen(const u8 *p) { return *(u32 *)p; }
static u8 *bl_token(u8 *p) { return p + 4; }
static const u8 *bl_token_c(const u8 *p) { return p + 4; }
static u32 bl_plen(const u8 *p) {
    u32 t = *(u32 *)p;
    return *(u32 *)(p + 4 + t);
}
static u8 *bl_payload(u8 *p) {
    u32 t = *(u32 *)p;
    return p + 8 + t;
}
static const u8 *bl_payload_c(const u8 *p) {
    u32 t = *(u32 *)p;
    return p + 8 + t;
}
static u32 bl_instr_size(const u8 *p) {
    u32 t = *(u32 *)p;
    if (t == 0) return 4;
    return 8 + t + *(u32 *)(p + 4 + t);
}
static int bl_is_end(const u8 *p) { return *(u32 *)p == 0; }
static int bl_zero32(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}
/* Walk: return 0 if cannot advance safely within n bytes from base. */
static int bl_ok(const u8 *base, u32 n, u32 o) {
    if (o + 4 > n) return 0;
    u32 t = *(u32 *)(base + o);
    if (t == 0) return 1;
    if (t > (1u << 20) || o + 8 + t > n) return 0;
    u32 pl = *(u32 *)(base + o + 4 + t);
    if (pl > (1u << 20) || o + 8 + t + pl > n) return 0;
    return 1;
}
/* Encode one instruction into dst; return bytes written. token_len must match. */
static u32 bl_write(u8 *dst, const u8 *token, u32 tlen, const u8 *payload, u32 plen) {
    *(u32 *)dst = tlen;
    if (tlen) memcpy(dst + 4, token, tlen);
    *(u32 *)(dst + 4 + tlen) = plen;
    if (plen) memcpy(dst + 8 + tlen, payload, plen);
    return 8 + tlen + plen;
}
static u32 bl_write_end(u8 *dst) {
    *(u32 *)dst = 0;
    return 4;
}
#endif
