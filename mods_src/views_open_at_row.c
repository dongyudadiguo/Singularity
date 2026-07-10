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
extern __declspec(dllimport) void cvm_var_write(const u8 *id, const u8 *data, u32 size);
extern __declspec(dllimport) int cvm_resolve_payload_hash(const H k, H h);
extern __declspec(dllimport) u8 *cvm_cached_base(void);
extern __declspec(dllimport) u32 cvm_cached_len(void);

#define VIEW_MAX 32
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

static int zero32(const u8 *p){for(int i=0;i<32;i++)if(p[i])return 0;return 1;}
static int same_key(const u8 *a,const u8 *b){for(int i=0;i<32;i++)if(a[i]!=b[i])return 0;return 1;}

/* payload: views_var[32]
 * stack: u32 view_index, u32 row, f32 x, f32 y
 * Opens token at row of view as linked view at (x,y). Activates+drags it.
 * No-op on invalid/end token. Pushes u32 index or 0xffffffff.
 */
__declspec(dllexport) void run(void) {
    u32 out = 0xffffffffu;
    float y = *(float*)pop(4);
    float x = *(float*)pop(4);
    u32 row = *(u32*)pop(4);
    u32 vi = *(u32*)pop(4);
    if (cvm_payload_size() < 32) { push(&out,4); cont(); return; }
    H id; memcpy(id, cvm_payload(), 32);
    u32 size=0; u8 *raw=cvm_var_get(id,&size);
    if(!raw || size < sizeof(Table)) { push(&out,4); cont(); return; }
    Table t; memcpy(&t, raw, sizeof(t));
    if (vi >= t.count || !t.views[vi].used) { push(&out,4); cont(); return; }

    H h;
    cvm_resolve_payload_hash(t.views[vi].key, h);
    u8 *b = cvm_cached_base();
    u32 n = cvm_cached_len();
    u32 o=0;
    for (u32 i=0; i<row && o+36<=n && !zero32(b+o); i++) {
        u32 pn=*(u32*)(b+o+32);
        if (o+36+pn>n) { o=n; break; }
        o += 36+pn;
    }
    u8 key[32]; memset(key,0,32);
    if (o+32<=n && !zero32(b+o)) memcpy(key,b+o,32);
    if (zero32(key)) { push(&out,4); cont(); return; }
    if (o+36<=n) {
        u32 pn=*(u32*)(b+o+32);
        if (pn==32 && o+68<=n) {
            u8 ph[32]; memcpy(ph,b+o+36,32);
            if (!zero32(ph)) memcpy(key,ph,32);
        }
    }

    /* existing? */
    for (u32 i=0;i<t.count;i++){
        if (t.views[i].used && same_key(t.views[i].key,key)) {
            t.active=i; t.dragging=(int)i;
            cvm_var_write(id,(u8*)&t,sizeof(t));
            out=i; push(&out,4); cont(); return;
        }
    }
    if (t.count < VIEW_MAX) {
        u32 i=t.count++;
        memset(&t.views[i],0,sizeof(View));
        t.views[i].used=1;
        memcpy(t.views[i].key,key,32);
        t.views[i].x=x; t.views[i].y=y;
        t.views[i].parent=(int)vi;
        t.views[i].linked=1;
        t.views[i].link_x=80.0f;
        t.views[i].link_y=row*24.0f + 10.0f;
        t.views[i].parent=(int)vi;
        t.active=i; t.dragging=(int)i;
        out=i;
        cvm_var_write(id,(u8*)&t,sizeof(t));
    }
    push(&out,4);
    cont();
}
