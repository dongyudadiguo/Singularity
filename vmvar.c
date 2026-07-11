#include <windows.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char u8;
typedef unsigned u32;

#define MAX_VARS 256
#define MAX_SCOPES 64
#define MAX_ID_LEN 256

typedef struct {
    u8 *id;
    u32 id_len;
    u8 *data;
    u32 size;
    u32 scope;
    u8 used;
} Var;

static Var vars[MAX_VARS];
static u32 scope_stack[MAX_SCOPES];
static u32 scope_depth;
static u32 current_scope_id;

static u32 hash_id(const u8 *id, u32 id_len) {
    u32 h = 2166136261u;
    for (u32 i = 0; i < id_len; i++) { h ^= id[i]; h *= 16777619u; }
    return h % MAX_VARS;
}

static int id_eq(const Var *v, const u8 *id, u32 id_len) {
    return v->used && v->id_len == id_len && (id_len == 0 || !memcmp(v->id, id, id_len));
}

static void free_var(Var *v) {
    free(v->id); v->id = 0; v->id_len = 0;
    free(v->data); v->data = 0; v->size = 0;
    v->used = 0;
}

__declspec(dllexport) void cvm_scope_start(void) {
    if (scope_depth < MAX_SCOPES) {
        scope_stack[scope_depth++] = current_scope_id;
        current_scope_id++;
    }
}

__declspec(dllexport) void cvm_scope_end(void) {
    for (int i = 0; i < MAX_VARS; i++) {
        if (vars[i].used && vars[i].scope == current_scope_id) free_var(&vars[i]);
    }
    if (scope_depth > 0) current_scope_id = scope_stack[--scope_depth];
}

/* id may be any binary blob of id_len bytes (0..MAX_ID_LEN). */
__declspec(dllexport) u8 *cvm_var_get(const u8 *id, u32 id_len, u32 *size) {
    if (!id && id_len) return 0;
    if (id_len > MAX_ID_LEN) return 0;
    u32 idx = hash_id(id, id_len);
    for (int i = 0; i < MAX_VARS; i++) {
        u32 cur = (idx + i) % MAX_VARS;
        if (!vars[cur].used) return 0;
        if (id_eq(&vars[cur], id, id_len)) {
            if (size) *size = vars[cur].size;
            return vars[cur].data;
        }
    }
    return 0;
}

__declspec(dllexport) void cvm_var_set(const u8 *id, u32 id_len, u32 size) {
    if (!id && id_len) return;
    if (id_len > MAX_ID_LEN) return;
    u32 idx = hash_id(id, id_len);
    for (int i = 0; i < MAX_VARS; i++) {
        u32 cur = (idx + i) % MAX_VARS;
        if (!vars[cur].used) {
            vars[cur].id = (u8*)malloc(id_len ? id_len : 1);
            if (!vars[cur].id) return;
            if (id_len) memcpy(vars[cur].id, id, id_len);
            vars[cur].id_len = id_len;
            vars[cur].data = (u8*)malloc(size ? size : 1);
            if (!vars[cur].data) { free(vars[cur].id); vars[cur].id = 0; return; }
            memset(vars[cur].data, 0, size ? size : 1);
            vars[cur].size = size;
            vars[cur].scope = current_scope_id;
            vars[cur].used = 1;
            return;
        }
        if (id_eq(&vars[cur], id, id_len)) {
            free(vars[cur].data);
            vars[cur].data = (u8*)malloc(size ? size : 1);
            if (!vars[cur].data) { vars[cur].size = 0; return; }
            memset(vars[cur].data, 0, size ? size : 1);
            vars[cur].size = size;
            vars[cur].scope = current_scope_id;
            return;
        }
    }
}

__declspec(dllexport) void cvm_var_write(const u8 *id, u32 id_len, const u8 *data, u32 size) {
    if (!id && id_len) return;
    if (id_len > MAX_ID_LEN) return;
    u32 idx = hash_id(id, id_len);
    for (int i = 0; i < MAX_VARS; i++) {
        u32 cur = (idx + i) % MAX_VARS;
        if (!vars[cur].used) return;
        if (id_eq(&vars[cur], id, id_len)) {
            if (size > vars[cur].size) size = vars[cur].size;
            if (size && data) memcpy(vars[cur].data, data, size);
            return;
        }
    }
}

/* Back-compat helpers: fixed 32-byte id (legacy call sites). */
__declspec(dllexport) u8 *cvm_var_get32(const u8 id[32], u32 *size) {
    return cvm_var_get(id, 32, size);
}
__declspec(dllexport) void cvm_var_set32(const u8 id[32], u32 size) {
    cvm_var_set(id, 32, size);
}
__declspec(dllexport) void cvm_var_write32(const u8 id[32], const u8 *data, u32 size) {
    cvm_var_write(id, 32, data, size);
}
