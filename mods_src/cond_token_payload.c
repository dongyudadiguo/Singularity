typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *from(u32 size);
extern __declspec(dllimport) u8 *cvm_payload(void);
extern __declspec(dllimport) u32 cvm_payload_size(void);
extern __declspec(dllimport) void cvm_exec(const H h);
extern __declspec(dllimport) void cvm_heat_pulse(u32 uid, const H node_key);

static int mod_bool(const void *p) {
    const u8 *b = (const u8*)p;
    for (u32 i = 0; i < 4; i++) if (b[i]) return 1;
    return 0;
}

#define ONCE_CAP 256
static struct { u32 uid; u8 prev; u8 on; } g_once[ONCE_CAP];

static int once_should_fire(u32 uid, int cur) {
    int free_i = -1;
    if (!uid) return cur;
    for (int i = 0; i < ONCE_CAP; i++) {
        if (g_once[i].on && g_once[i].uid == uid) {
            int edge = cur && !g_once[i].prev;
            g_once[i].prev = (u8)(cur ? 1 : 0);
            return edge;
        }
        if (!g_once[i].on && free_i < 0) free_i = i;
    }
    {
        int i = free_i >= 0 ? free_i : 0;
        g_once[i].on = 1;
        g_once[i].uid = uid;
        g_once[i].prev = (u8)(cur ? 1 : 0);
        return cur;
    }
}

/*
 * payload: token[32] + uid[u32] + once[u8] + continuous[u8] + pad[2]
 * Heat: pulse TARGET token as node_key so the opened/target node lights up.
 */
__declspec(dllexport) void run(void) {
    H h;
    int ok = mod_bool(from(4));
    u8 *p = cvm_payload();
    u32 n = cvm_payload_size();
    u32 uid = 0;
    u8 once = 0, continuous = 0;
    int fire = 0;

    if (n < 32) { cont(); return; }
    for (u32 i = 0; i < 32; i++) h[i] = p[i];
    if (n >= 36) uid = *(u32*)(p + 32);
    if (n >= 38) { once = p[36]; continuous = p[37]; }
    if (!once && !continuous) continuous = 1;

    if (!ok) {
        if (once) once_should_fire(uid, 0);
        cont();
        return;
    }

    if (continuous) fire = 1;
    else if (once) fire = once_should_fire(uid, 1);
    else fire = 1;

    if (fire) {
        /* target node heat */
        cvm_heat_pulse(uid ? uid : 1, h);
        cvm_exec(h);
    } else cont();
}
