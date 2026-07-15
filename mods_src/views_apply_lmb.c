#include <string.h>
extern __declspec(dllimport) void cvm_hand_toggle(unsigned uid);
#include <string.h>
#include "views_common.h"
#include <stdlib.h>
typedef unsigned u32;
extern __declspec(dllimport) void *from(u32);
extern __declspec(dllimport) void *slot(u32);
extern __declspec(dllimport) int cvm_key_dirty(const H key);
extern __declspec(dllimport) void cvm_flush_key(const H key);
extern __declspec(dllimport) void cvm_vote(const H parent, const H child);

static int row_from_link_y(float link_y, float row_h) {
    int r = (int)((link_y - 10.0f) / row_h + 0.5f);
    if (r < 0) r = 0;
    return r;
}

static void vote_node_tokens(const View *v) {
    if (key_is_tag(v->key)) {
        /* Tag explorer: vote node -> each child */
        H kids[256]; H p; memcpy(p, v->key, 32);
        u32 kc = cvm_children(p, kids, 256);
        if (kc > 256) kc = 256;
        H parent; memcpy(parent, v->key, 32);
        for (u32 i = 0; i < kc; i++) {
            if (zero_key(kids[i]) || same_key(kids[i], parent)) continue;
            cvm_vote(parent, kids[i]);
        }
        return;
    }
    H h; cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 n = cvm_cached_len();
    u32 o = 0;
    H parent; memcpy(parent, v->key, 32);
    while (bl_ok(b, n, o) && !bl_is_end(b + o)) {
        u32 tlen = bl_tlen(b + o);
        const u8 *tok = bl_token_c(b + o);
        if (tlen == 32) {
            H child; memcpy(child, tok, 32);
            cvm_vote(parent, child);
        }
        o += bl_instr_size(b + o);
    }
}

__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    float my=*(float*)from(4), mx=*(float*)from(4);
    float title_h=32.f, row_h=24.f; u32 row_count=256;
    if (an>=4) title_h=*(float*)args;
    if (an>=8) row_h=*(float*)(args+4);
    if (an>=12) row_count=*(u32*)(args+8);
    u32 handled=0;
    Table *tp=load_table(id, id_len); if(!tp){ memcpy(slot(4), &handled, 4); cont(); return; }
    Table t=*tp;

        /* Mid-link: token_run_by_hand arm toggle only */
    for (u32 vi = 0; vi < t.count; vi++) {
        View *v = &t.views[vi];
        if (!v->used || !v->linked || v->parent < 0) continue;
        if ((u32)v->parent >= t.count || !t.views[v->parent].used) continue;
        View *p = &t.views[v->parent];
        if (key_is_tag(p->key) || key_is_tag(v->key)) continue;
        float x1 = view_draw_x(&t, (u32)v->parent) + v->link_x, y1 = p->y + v->link_y;
        float x2 = view_draw_x(&t, vi), y2 = v->y;
        float midx = (x1 + x2) * 0.5f, midy = (y1 + y2) * 0.5f;
        u32 prow = (u32)row_from_link_y(v->link_y, row_h);
        const u8 *instr = row_instr(p, prow, 0);
        if (!instr || bl_tlen(instr) != 32) continue;
        const char *nm = token_name(bl_token_c(instr));
        u32 pn = bl_plen(instr);
        if (!nm || strcmp(nm, "token_run_by_hand") || pn < 36) continue;
        u32 uid = *(u32*)bl_payload_c(instr);
        float bw = 36.0f, bh = 18.0f;
        float bx = midx - bw * 0.5f, by = midy - bh * 0.5f;
        if (mx >= bx && mx < bx + bw && my >= by && my < by + bh) {
            cvm_hand_toggle(uid);
            handled = 1;
            store_table(id, id_len, &t);
            memcpy(slot(4), &handled, 4);
            cont(); return;
        }
    }

    /* Header buttons */
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        int is_tag = key_is_tag(v->key);
        int dirty = (!is_tag) ? cvm_key_dirty(v->key) : 0;
        int show_rec = is_tag || !dirty;
        int show_latch = !is_tag;
        int btn = header_btn_hit(&t, (u32)i, v, mx, my, title_h, dirty, show_rec, show_latch);
        if (!btn) continue;
        t.active = (u32)i;
        t.dragging = -1;
        if (btn == 1) {
            cvm_flush_key(v->key);
        } else if (btn == 2 && show_latch) {
            t.views[i].pad0 ^= 1u;
        } else if (btn == 3) {
            vote_node_tokens(v);
        }
        handled = 1;
        store_table(id, id_len, &t);
        memcpy(slot(4), &handled, 4); cont(); return;
    }

    /* Title drag */
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float width=title_text_width((u32)i,v);
        int is_tag = key_is_tag(v->key);
        int dirty = (!is_tag) ? cvm_key_dirty(v->key) : 0;
        int show_rec = is_tag || !dirty;
        float dx = view_draw_x(&t, (u32)i);
        float hw = width + 120.0f;
        if (mx>=dx && mx<dx+hw && my>=v->y-title_h && my<v->y) {
            t.active=(u32)i;
            t.dragging=i;
            handled=1;
            store_table(id, id_len, &t);
            memcpy(slot(4), &handled, 4); cont(); return;
        }
    }

    /* Row select — always allow click-select */
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float rel=my-v->y; if(rel<0) continue;
        int row=(int)(rel/row_h);
        if (row<0 || (u32)row>row_count) continue;
        float width=row_hit_width(v,row);
        float dx = view_draw_x(&t, (u32)i);
        if (mx<dx || mx>=dx+width) continue;
        t.active=(u32)i;
        t.views[i].cursor=(u32)row;
        t.dragging=-1;
        handled=1;
        store_table(id, id_len, &t);
        memcpy(slot(4), &handled, 4); cont(); return;
    }

    if (t.dragging != -1) {
        t.dragging = -1;
        store_table(id, id_len, &t);
    }
    memcpy(slot(4), &handled, 4); cont();
}
