#include "views_common.h"
#include <stdlib.h>
extern __declspec(dllimport) int cvm_key_dirty(const H key);
extern __declspec(dllimport) void cvm_flush_key(const H key);
extern __declspec(dllimport) void cvm_vote(const H parent, const H child);

static int row_from_link_y(float link_y, float row_h) {
    int r = (int)((link_y - 10.0f) / row_h + 0.5f);
    if (r < 0) r = 0;
    return r;
}

static void vote_node_tokens(const View *v) {
    H h; cvm_resolve_payload_hash(v->key, h);
    u8 *b = cvm_cached_base();
    u32 n = cvm_cached_len();
    u32 o = 0;
    H parent; memcpy(parent, v->key, 32);
    while (o + 36 <= n && !zero_key(b + o)) {
        u32 pn = *(u32*)(b + o + 32);
        if (o + 36 + pn > n) break;
        H child; memcpy(child, b + o, 32);
        cvm_vote(parent, child);
        o += 36 + pn;
    }
}

/* Toggle once/continuous on cond_token_payload at parent row. */
static int toggle_cond_flag(View *parent, u32 row, int which) {
    if (key_is_tag(parent->key)) return 0;
    H h; cvm_resolve_payload_hash(parent->key, h);
    u8 *b = cvm_cached_base();
    u32 n = cvm_cached_len();
    u32 o = block_row_offset(parent, row);
    if (o + 36 > n) return 0;
    const char *nm = token_name(b + o);
    u32 pn = *(u32*)(b + o + 32);
    if (!nm || strcmp(nm, "cond_token_payload") || pn < 38) return 0;
    u8 *pay = b + o + 36;
    if (which == 1) {
        pay[36] = pay[36] ? 0 : 1; /* once toggle */
    } else if (which == 2) {
        pay[37] = pay[37] ? 0 : 1; /* continuous toggle */
        if (pay[37]) pay[36] = 0; /* continuous hides/clears once */
    }
    return 1;
}

/* stack: mx,my; payload: id + title_h,row_h,row_count
 * title buttons: commit / latch / vote (before title-drag)
 * mid-link cond buttons
 * title -> set active + dragging
 * row   -> select_row
 */
__declspec(dllexport) void run(void){
    const u8 *id; u32 id_len; const u8 *args; u32 an;
    if (!payload_id(&id, &id_len, &args, &an)) { cont(); return; }
    float my=*(float*)pop(4), mx=*(float*)pop(4);
    float title_h=32.f, row_h=24.f; u32 row_count=256;
    if (an>=4) title_h=*(float*)args;
    if (an>=8) row_h=*(float*)(args+4);
    if (an>=12) row_count=*(u32*)(args+8);
    u32 handled=0;
    Table *tp=load_table(id, id_len); if(!tp){ push(&handled,4); cont(); return; }
    Table t=*tp;

    /* 1) Mid-link cond buttons (once / continuous) */
    for (u32 vi = 0; vi < t.count; vi++) {
        View *v = &t.views[vi];
        if (!v->used || !v->linked || v->parent < 0) continue;
        if ((u32)v->parent >= t.count || !t.views[v->parent].used) continue;
        View *p = &t.views[v->parent];
        float x1 = p->x + v->link_x, y1 = p->y + v->link_y;
        float x2 = v->x, y2 = v->y;
        float midx = (x1 + x2) * 0.5f, midy = (y1 + y2) * 0.5f;
        u32 prow = (u32)row_from_link_y(v->link_y, row_h);
        if (key_is_tag(p->key)) continue;
        const u8 *instr = row_instr(p, prow, 0);
        if (!instr) continue;
        const char *nm = token_name(instr);
        u32 pn = *(u32*)(instr + 32);
        if (!nm || strcmp(nm, "cond_token_payload") || pn < 38) continue;
        u8 once = 0, contf = 0;
        cond_token_parse(instr + 36, pn, 0, 0, &once, &contf);
        float bx = midx - 28.0f, by = midy - 9.0f;
        if (!contf) {
            if (mx >= bx && mx < bx + 28.0f && my >= by && my < by + 18.0f) {
                toggle_cond_flag(p, prow, 1);
                handled = 1;
                store_table(id, id_len, &t);
                push(&handled,4); cont(); return;
            }
            bx += 32.0f;
        }
        if (mx >= bx && mx < bx + 28.0f && my >= by && my < by + 18.0f) {
            toggle_cond_flag(p, prow, 2);
            handled = 1;
            store_table(id, id_len, &t);
            push(&handled,4); cont(); return;
        }
    }

    /* 2) Header buttons (commit / latch / vote) — no drag */
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        int dirty = (!key_is_tag(v->key)) ? cvm_key_dirty(v->key) : 0;
        int show_rec = !dirty && !key_is_tag(v->key);
        int btn = header_btn_hit((u32)i, v, mx, my, title_h, dirty, show_rec);
        if (!btn) continue;
        t.active = (u32)i;
        t.dragging = -1;
        if (btn == 1) {
            /* commit once */
            cvm_flush_key(v->key);
        } else if (btn == 2) {
            t.views[i].pad0 ^= 1u;
        } else if (btn == 3) {
            vote_node_tokens(v);
        }
        handled = 1;
        store_table(id, id_len, &t);
        push(&handled,4); cont(); return;
    }

    /* 3) Title drag */
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float width=title_text_width((u32)i,v);
        /* extend hit to include buttons so empty title area still works */
        int dirty = (!key_is_tag(v->key)) ? cvm_key_dirty(v->key) : 0;
        int show_rec = !dirty && !key_is_tag(v->key);
        float hw = header_total_width((u32)i, v, dirty, show_rec);
        if (hw > width) width = hw;
        if (mx>=v->x && mx<v->x+width && my>=v->y-title_h && my<v->y) {
            t.active=(u32)i;
            t.dragging=i;
            handled=1;
            store_table(id, id_len, &t);
            push(&handled,4); cont(); return;
        }
    }

    /* 4) Row select */
    for (int i=(int)t.count-1;i>=0;i--) {
        View *v=&t.views[i]; if(!v->used) continue;
        float rel=my-v->y; if(rel<0) continue;
        int row=(int)(rel/row_h);
        if (row<0 || (u32)row>row_count) continue;
        float width=row_hit_width(v,row);
        if (mx<v->x || mx>=v->x+width) continue;
        t.active=(u32)i;
        t.views[i].cursor=(u32)row;
        t.dragging=-1;
        handled=1;
        store_table(id, id_len, &t);
        push(&handled,4); cont(); return;
    }

    if (t.dragging != -1) {
        t.dragging = -1;
        store_table(id, id_len, &t);
    }
    push(&handled,4); cont();
}
