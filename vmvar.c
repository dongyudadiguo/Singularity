#include <windows.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

#define MAX_VARS 256
#define MAX_SCOPES 64

typedef struct {
    H id;
    u8 *data;
    u32 size;
    u32 scope;
    u8 used;
} Var;

static Var vars[MAX_VARS];
static u32 scope_stack[MAX_SCOPES];
static u32 scope_depth;
static u32 current_scope_id;

static u32 hash_id(const H id) {
    u32 h = 0;
    for (int i = 0; i < 4; i++) h = h * 31 + id[i];
    return h % MAX_VARS;
}

__declspec(dllexport) void cvm_scope_start(void) {
    if (scope_depth < MAX_SCOPES) {
        scope_stack[scope_depth++] = current_scope_id;
        current_scope_id++;
    }
}

__declspec(dllexport) void cvm_scope_end(void) {
    for (int i = 0; i < MAX_VARS; i++) {
        if (vars[i].used && vars[i].scope == current_scope_id) {
            free(vars[i].data);
            vars[i].used = 0;
        }
    }
    if (scope_depth > 0) {
        current_scope_id = scope_stack[--scope_depth];
    }
}

__declspec(dllexport) u8 *cvm_var_get(const H id, u32 *size) {
    u32 idx = hash_id(id);
    for (int i = 0; i < MAX_VARS; i++) {
        u32 cur = (idx + i) % MAX_VARS;
        if (!vars[cur].used) return 0;
        if (!memcmp(vars[cur].id, id, 32)) {
            *size = vars[cur].size;
            return vars[cur].data;
        }
    }
    return 0;
}

__declspec(dllexport) void cvm_var_set(const H id, u32 size) {
    u32 idx = hash_id(id);
    for (int i = 0; i < MAX_VARS; i++) {
        u32 cur = (idx + i) % MAX_VARS;
        if (!vars[cur].used) {
            memcpy(vars[cur].id, id, 32);
            vars[cur].data = malloc(size);
            memset(vars[cur].data, 0, size);
            vars[cur].size = size;
            vars[cur].scope = current_scope_id;
            vars[cur].used = 1;
            return;
        }
        if (!memcmp(vars[cur].id, id, 32)) {
            free(vars[cur].data);
            vars[cur].data = malloc(size);
            memset(vars[cur].data, 0, size);
            vars[cur].size = size;
            vars[cur].scope = current_scope_id;
            return;
        }
    }
}

__declspec(dllexport) void cvm_var_write(const H id, const u8 *data, u32 size) {
    u32 idx = hash_id(id);
    for (int i = 0; i < MAX_VARS; i++) {
        u32 cur = (idx + i) % MAX_VARS;
        if (!vars[cur].used) return;
        if (!memcmp(vars[cur].id, id, 32)) {
            if (size > vars[cur].size) size = vars[cur].size;
            memcpy(vars[cur].data, data, size);
            return;
        }
    }
}