#include <string.h>
typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(u32 size);
extern __declspec(dllimport) void push(const void *p, u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) u8 *cvm_var_get(const u8 *id, u32 *size);
extern __declspec(dllimport) void cvm_var_set(const u8 *id, u32 size);
extern __declspec(dllimport) void cvm_var_write(const u8 *id, const u8 *data, u32 size);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);

/* Compact multi-view table stored in a single VM var.
 *
 * Layout:
 *   u32 view_count
 *   u32 active
 *   i32 dragging   (-1 none)
 *   u32 _pad
 *   View views[MAX]
 *
 * View:
 *   key[32]
 *   f32 x, y
 *   i32 parent     (-1 none)
 *   i32 linked     (0/1)
 *   f32 link_x, link_y
 *   u32 used
 *   u32 cursor     (row index into block)
 *   u32 _pad0,_pad1
 *
 * Payload:
 *   var_id[32] + u32 op [+ optional fixed args]
 *
 * Ops:
 *  0 init        args: key[32], f32 x, f32 y
 *  1 count       -> u32
 *  2 active      -> u32
 *  3 set_active  args: u32 index
 *  4 get_xy      args: u32 index -> f32 x, f32 y
 *  5 set_xy      args: u32 index, f32 x, f32 y
 *  6 get_key     args: u32 index -> key[32]
 *  7 get_parent  args: u32 index -> i32
 *  8 get_link    args: u32 index -> f32 lx, f32 ly, i32 linked
 *  9 get_cursor  args: u32 index -> u32
 * 10 set_cursor  args: u32 index, u32 cursor
 * 11 drag_begin  args: u32 index
 * 12 drag_step   args: f32 dx, f32 dy   (world delta already /zoom)
 * 13 drag_end
 * 14 open        args: key[32], f32 x, f32 y, i32 parent, f32 lx, f32 ly
 *                if key already open: activate that view
 *                else create linked view, activate it, begin drag
 *                -> u32 index (0xffffffff on fail)
 * 15 hit_title   stack: f32 mx,my  args: f32 title_h, f32 width
 *                -> i32 index or -1  (topmost)
 * 16 hit_row     stack: f32 mx,my  args: f32 row_h, f32 width, u32 row_count
 *                -> i32 view, i32 row  (-1,-1 miss)
 * 17 move_by     args: u32 index, f32 dx, f32 dy
 * 18 ensure      args: key[32], f32 x, f32 y  (init if empty else no-op)
 */

#define VIEW_MAX 32
#define VIEW_SIZE 72
#define HEADER_SIZE 16
#define BLOB_SIZE (HEADER_SIZE + VIEW_MAX * VIEW_SIZE)

typedef struct {
    u8 key[32];
    float x, y;
    int parent;
    int linked;
    float link_x, link_y;
    u32 used;
    u32 cursor;
    u32 pad0, pad1;
} View;

typedef struct {
    u32 count;
    u32 active;
    int dragging;
    u32 pad;
    View views[VIEW_MAX];
} Table;

static u8 *blob(const H id, u32 *size_out) {
    u32 size = 0;
    u8 *p = cvm_var_get(id, &size);
    if (!p || size < (u32)sizeof(Table)) return 0;
    if (size_out) *size_out = size;
    return p;
}

static Table *load(const H id) {
    return (Table *)blob(id, 0);
}

static void store(const H id, Table *t) {
    cvm_var_write(id, (const u8 *)t, (u32)sizeof(Table));
}

static int same_key(const u8 *a, const u8 *b) {
    for (int i = 0; i < 32; i++) if (a[i] != b[i]) return 0;
    return 1;
}

static int zero_key(const u8 *p) {
    for (int i = 0; i < 32; i++) if (p[i]) return 0;
    return 1;
}

static void zero_view(View *v) {
    memset(v, 0, sizeof(*v));
    v->parent = -1;
}

__declspec(dllexport) void run(void) {
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    if (n < 36) { cont(); return; }
    H id;
    memcpy(id, p, 32);
    u32 op = *(u32 *)(p + 32);
    const u8 *args = p + 36;
    u32 an = n - 36;

    if (op == 0) {
        /* init: create/resize and seed view0 */
        if (an < 40) { cont(); return; }
        cvm_var_set(id, (u32)sizeof(Table));
        Table t;
        memset(&t, 0, sizeof(t));
        t.dragging = -1;
        t.count = 1;
        t.active = 0;
        zero_view(&t.views[0]);
        t.views[0].used = 1;
        memcpy(t.views[0].key, args, 32);
        t.views[0].x = *(float *)(args + 32);
        t.views[0].y = *(float *)(args + 36);
        store(id, &t);
        cont();
        return;
    }

    Table *tp = load(id);
    if (!tp) {
        /* missing table: push safe defaults for read ops */
        if (op == 1 || op == 2) { u32 z = 0; push(&z, 4); }
        else if (op == 4) { float z = 0; push(&z, 4); push(&z, 4); }
        else if (op == 6) { u8 z[32] = {0}; push(z, 32); }
        else if (op == 7) { int z = -1; push(&z, 4); }
        else if (op == 8) { float z = 0; int l = 0; push(&z, 4); push(&z, 4); push(&l, 4); }
        else if (op == 9) { u32 z = 0; push(&z, 4); }
        else if (op == 14) { u32 z = 0xffffffffu; push(&z, 4); }
        else if (op == 15) { int z = -1; (void)pop(8); push(&z, 4); }
        else if (op == 16) { int z = -1; (void)pop(8); push(&z, 4); push(&z, 4); }
        cont();
        return;
    }
    Table t = *tp;

    if (op == 1) {
        push(&t.count, 4);
    } else if (op == 2) {
        push(&t.active, 4);
    } else if (op == 3) {
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) t.active = idx;
        }
        store(id, &t);
    } else if (op == 4) {
        float x = 0, y = 0;
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) { x = t.views[idx].x; y = t.views[idx].y; }
        }
        push(&x, 4); push(&y, 4);
    } else if (op == 5) {
        if (an >= 12) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) {
                t.views[idx].x = *(float *)(args + 4);
                t.views[idx].y = *(float *)(args + 8);
            }
        }
        store(id, &t);
    } else if (op == 6) {
        u8 key[32]; memset(key, 0, 32);
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) memcpy(key, t.views[idx].key, 32);
        }
        push(key, 32);
    } else if (op == 7) {
        int parent = -1;
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) parent = t.views[idx].parent;
        }
        push(&parent, 4);
    } else if (op == 8) {
        float lx = 0, ly = 0; int linked = 0;
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) {
                lx = t.views[idx].link_x; ly = t.views[idx].link_y; linked = t.views[idx].linked;
            }
        }
        push(&lx, 4); push(&ly, 4); push(&linked, 4);
    } else if (op == 9) {
        u32 cur = 0;
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) cur = t.views[idx].cursor;
        }
        push(&cur, 4);
    } else if (op == 10) {
        if (an >= 8) {
            u32 idx = *(u32 *)args;
            u32 cur = *(u32 *)(args + 4);
            if (idx < t.count && t.views[idx].used) t.views[idx].cursor = cur;
        }
        store(id, &t);
    } else if (op == 11) {
        if (an >= 4) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) {
                t.dragging = (int)idx;
                t.active = idx;
            }
        }
        store(id, &t);
    } else if (op == 12) {
        float dx = 0.0f, dy = 0.0f;
        if (an >= 8) { dx = *(float *)args; dy = *(float *)(args + 4); }
        else { dy = *(float *)pop(4); dx = *(float *)pop(4); }
        if (t.dragging >= 0 && (u32)t.dragging < t.count && t.views[t.dragging].used) {
            t.views[t.dragging].x += dx;
            t.views[t.dragging].y += dy;
        }
        store(id, &t);
    } else if (op == 13) {
        t.dragging = -1;
        store(id, &t);
    } else if (op == 14) {
        /* open/create linked view */
        u32 out = 0xffffffffu;
        if (an >= 56) {
            const u8 *key = args;
            float x = *(float *)(args + 32);
            float y = *(float *)(args + 36);
            int parent = *(int *)(args + 40);
            float lx = *(float *)(args + 44);
            float ly = *(float *)(args + 48);
            /* find existing */
            for (u32 i = 0; i < t.count; i++) {
                if (t.views[i].used && same_key(t.views[i].key, key)) {
                    t.active = i;
                    t.dragging = (int)i;
                    out = i;
                    store(id, &t);
                    push(&out, 4);
                    cont();
                    return;
                }
            }
            if (t.count < VIEW_MAX) {
                u32 i = t.count++;
                zero_view(&t.views[i]);
                t.views[i].used = 1;
                memcpy(t.views[i].key, key, 32);
                t.views[i].x = x;
                t.views[i].y = y;
                t.views[i].parent = parent;
                t.views[i].linked = 1;
                t.views[i].link_x = lx;
                t.views[i].link_y = ly;
                t.active = i;
                t.dragging = (int)i;
                out = i;
            }
        }
        store(id, &t);
        push(&out, 4);
    } else if (op == 15) {
        /* hit title bar: stack mx,my ; args title_h,width */
        float my = *(float *)pop(4);
        float mx = *(float *)pop(4);
        float title_h = 32.0f, width = 520.0f;
        if (an >= 8) { title_h = *(float *)args; width = *(float *)(args + 4); }
        int hit = -1;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            if (mx >= v->x && mx < v->x + width && my >= v->y - title_h && my < v->y) {
                hit = i; break;
            }
        }
        push(&hit, 4);
    } else if (op == 16) {
        /* hit row: stack mx,my ; args row_h,width,row_count */
        float my = *(float *)pop(4);
        float mx = *(float *)pop(4);
        float row_h = 24.0f, width = 520.0f; u32 row_count = 64;
        if (an >= 12) {
            row_h = *(float *)args;
            width = *(float *)(args + 4);
            row_count = *(u32 *)(args + 8);
        }
        int vhit = -1, rhit = -1;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            if (mx < v->x || mx >= v->x + width) continue;
            float rel = my - v->y;
            if (rel < 0.0f) continue;
            int row = (int)(rel / row_h);
            if (row < 0 || (u32)row >= row_count) continue;
            vhit = i; rhit = row; break;
        }
        push(&vhit, 4); push(&rhit, 4);
    } else if (op == 17) {
        if (an >= 12) {
            u32 idx = *(u32 *)args;
            if (idx < t.count && t.views[idx].used) {
                t.views[idx].x += *(float *)(args + 4);
                t.views[idx].y += *(float *)(args + 8);
            }
        }
        store(id, &t);
    } else if (op == 18) {
        /* ensure seeded */
        if (an >= 40 && t.count == 0) {
            t.dragging = -1;
            t.count = 1;
            t.active = 0;
            zero_view(&t.views[0]);
            t.views[0].used = 1;
            memcpy(t.views[0].key, args, 32);
            t.views[0].x = *(float *)(args + 32);
            t.views[0].y = *(float *)(args + 36);
            store(id, &t);
        }
    } else if (op == 19) {
        /* get_dragging -> i32 */
        push(&t.dragging, 4);
    } else if (op == 20) {
        /* set_cursor_active: args u32 cursor on active view */
        if (an >= 4 && t.active < t.count && t.views[t.active].used) {
            t.views[t.active].cursor = *(u32 *)args;
            store(id, &t);
        }
    } else if (op == 21) {
        /* active_cursor -> u32 */
        u32 cur = 0;
        if (t.active < t.count && t.views[t.active].used) cur = t.views[t.active].cursor;
        push(&cur, 4);
    } else if (op == 22) {
        /* active_key -> key[32] */
        u8 key[32]; memset(key, 0, 32);
        if (t.active < t.count && t.views[t.active].used) memcpy(key, t.views[t.active].key, 32);
        push(key, 32);
    } else if (op == 23) {
        /* add_cursor_active: args i32 delta (can be negative via bit pattern) */
        if (an >= 4 && t.active < t.count && t.views[t.active].used) {
            int delta = *(int *)args;
            int cur = (int)t.views[t.active].cursor + delta;
            if (cur < 0) cur = 0;
            t.views[t.active].cursor = (u32)cur;
            store(id, &t);
        }
    } else if (op == 24) {
        /* dec_cursor_active sat */
        if (t.active < t.count && t.views[t.active].used) {
            if (t.views[t.active].cursor > 0) t.views[t.active].cursor--;
            store(id, &t);
        }
    } else if (op == 25) {
        /* set_active_stack: pop i32 */
        int idx = *(int *)pop(4);
        if (idx >= 0 && (u32)idx < t.count && t.views[idx].used) t.active = (u32)idx;
        store(id, &t);
    } else if (op == 26) {
        /* drag_begin_stack: pop i32 */
        int idx = *(int *)pop(4);
        if (idx >= 0 && (u32)idx < t.count && t.views[idx].used) {
            t.dragging = idx;
            t.active = (u32)idx;
        }
        store(id, &t);
    } else if (op == 27) {
        /* set_cursor_stack: pop i32 row */
        int row = *(int *)pop(4);
        if (row >= 0 && t.active < t.count && t.views[t.active].used) {
            t.views[t.active].cursor = (u32)row;
            store(id, &t);
        }
    } else if (op == 29) {
        /* pointer_lmb: pop my,mx. title->activate+drag; row->activate+cursor */
        float my = *(float *)pop(4);
        float mx = *(float *)pop(4);
        float title_h = 32.0f, width = 520.0f, row_h = 24.0f;
        u32 row_count = 256;
        if (an >= 16) {
            title_h = *(float *)args;
            width = *(float *)(args + 4);
            row_h = *(float *)(args + 8);
            row_count = *(u32 *)(args + 12);
        }
        int handled = 0;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            if (mx >= v->x && mx < v->x + width && my >= v->y - title_h && my < v->y) {
                t.active = (u32)i;
                handled = 1;
                break;
            }
        }
        if (!handled) {
            for (int i = (int)t.count - 1; i >= 0; i--) {
                View *v = &t.views[i];
                if (!v->used) continue;
                if (mx < v->x || mx >= v->x + width) continue;
                float rel = my - v->y;
                if (rel < 0.0f) continue;
                int row = (int)(rel / row_h);
                if (row < 0 || (u32)row > row_count) continue;
                t.active = (u32)i;
                t.views[i].cursor = (u32)row;
                handled = 1;
                break;
            }
        }
        store(id, &t);
        push(&handled, 4);
    } else if (op == 30) {
        /* pointer_rmb: pop my,mx. title->drag; row->open linked view at mouse */
        float my = *(float *)pop(4);
        float mx = *(float *)pop(4);
        float title_h = 32.0f, width = 520.0f, row_h = 24.0f;
        u32 row_count = 256;
        if (an >= 16) {
            title_h = *(float *)args;
            width = *(float *)(args + 4);
            row_h = *(float *)(args + 8);
            row_count = *(u32 *)(args + 12);
        }
        u32 handled = 0;
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            if (mx >= v->x && mx < v->x + width && my >= v->y - title_h && my < v->y) {
                t.active = (u32)i;
                t.dragging = i;
                handled = 1;
                store(id, &t);
                push(&handled, 4);
                cont();
                return;
            }
        }
        for (int i = (int)t.count - 1; i >= 0; i--) {
            View *v = &t.views[i];
            if (!v->used) continue;
            if (mx < v->x || mx >= v->x + width) continue;
            float rel = my - v->y;
            if (rel < 0.0f) continue;
            int row = (int)(rel / row_h);
            if (row < 0 || (u32)row >= row_count) continue;
            H h;
            cvm_resolve_payload_hash(v->key, h);
            u8 *b = cvm_cached_base();
            u32 nlen = cvm_cached_len();
            u32 o = 0;
            for (u32 r = 0; r < (u32)row && o + 36 <= nlen; r++) {
                if (zero_key(b + o)) { o = nlen; break; }
                u32 pn = *(u32 *)(b + o + 32);
                if (o + 36 + pn > nlen) { o = nlen; break; }
                o += 36 + pn;
            }
            u8 key[32];
            memset(key, 0, 32);
            if (o + 32 <= nlen && !zero_key(b + o)) memcpy(key, b + o, 32);
            if (zero_key(key)) break;
            int found = -1;
            for (u32 j = 0; j < t.count; j++) {
                if (t.views[j].used && same_key(t.views[j].key, key)) { found = (int)j; break; }
            }
            if (found >= 0) {
                t.active = (u32)found;
                t.dragging = found;
            } else if (t.count < VIEW_MAX) {
                u32 ni = t.count++;
                zero_view(&t.views[ni]);
                t.views[ni].used = 1;
                memcpy(t.views[ni].key, key, 32);
                t.views[ni].x = mx;
                t.views[ni].y = my;
                t.views[ni].parent = i;
                t.views[ni].linked = 1;
                t.views[ni].link_x = 80.0f;
                t.views[ni].link_y = (float)row * row_h + 10.0f;
                t.active = ni;
                t.dragging = (int)ni;
            }
            handled = 1;
            break;
        }
        store(id, &t);
        push(&handled, 4);
    }
    cont();
}
