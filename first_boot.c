#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];

#define DEFAULT_ADDR "118.25.42.70"
#define DEFAULT_PORT 9000
#define DEFAULT_BOOT_HASH "e2618d6b4e77b5d8b493f830ed67c22e15a61a7ffc84a2c70bb47897d19c515f"

static const H BOOT_KEY = {0x43,0x56,0x4d,0x5f,0x42,0x4f,0x4f,0x54};

static SOCKET conn = INVALID_SOCKET;

static void zero(H h) { memset(h, 0, 32); }

static int hex_val(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int hex_to_h(const char *hex, H out) {
    if (!hex || strlen(hex) != 64) return 0;
    for (int i = 0; i < 32; i++) {
        int hi = hex_val(hex[i * 2]);
        int lo = hex_val(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return 0;
        out[i] = (u8)((hi << 4) | lo);
    }
    return 1;
}

static void h_to_hex(const H h, char out[65]) {
    for (int i = 0; i < 32; i++) sprintf(out + i * 2, "%02x", h[i]);
    out[64] = 0;
}

static int sha256(const u8 *data, u32 len, H out) {
    HCRYPTPROV hp = 0;
    HCRYPTHASH hh = 0;
    DWORD sz = 32;
    int ok = 0;
    if (CryptAcquireContextW(&hp, 0, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hp, CALG_SHA_256, 0, 0, &hh)) {
            if (CryptHashData(hh, data, len, 0)) ok = !!CryptGetHashParam(hh, HP_HASHVAL, out, &sz, 0);
            CryptDestroyHash(hh);
        }
        CryptReleaseContext(hp, 0);
    }
    return ok;
}

static u8 *read_file(const char *path, u32 *out_len) {
    FILE *f = fopen(path, "rb");
    u8 *d;
    long n;
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return 0; }
    d = (u8*)malloc((size_t)n + 1);
    if (!d) { fclose(f); return 0; }
    if (n && fread(d, 1, (size_t)n, f) != (size_t)n) { free(d); fclose(f); return 0; }
    fclose(f);
    d[n] = 0;
    if (out_len) *out_len = (u32)n;
    return d;
}

static int read_hash_file(const char *path, H out) {
    u32 len = 0;
    u8 *d = read_file(path, &len);
    if (!d || len != 32) { free(d); return 0; }
    memcpy(out, d, 32);
    free(d);
    return 1;
}

static int send_all(const void *buf, u32 len) {
    const char *p = (const char*)buf;
    u32 sent = 0;
    while (sent < len) {
        int n = send(conn, p + sent, len - sent, 0);
        if (n <= 0) return 0;
        sent += (u32)n;
    }
    return 1;
}

static int readn(void *buf, u32 len) {
    char *p = (char*)buf;
    u32 got = 0;
    while (got < len) {
        int n = recv(conn, p + got, len - got, 0);
        if (n <= 0) return 0;
        got += (u32)n;
    }
    return 1;
}

static int request(u8 op, const void *body, u32 len, u8 *status, u8 **out, u32 *out_len) {
    u8 h[5] = {op, len >> 24, len >> 16, len >> 8, len};
    u32 l;
    if (!send_all(h, 5)) return 0;
    if (len && !send_all(body, len)) return 0;
    if (!readn(h, 5)) return 0;
    if (status) *status = h[0];
    l = (u32)h[1] << 24 | (u32)h[2] << 16 | (u32)h[3] << 8 | h[4];
    if (out_len) *out_len = l;
    if (out) {
        *out = (u8*)malloc(l + 1);
        if (!*out) return 0;
        if (l && !readn(*out, l)) { free(*out); *out = 0; return 0; }
        (*out)[l] = 0;
    } else {
        while (l) {
            char tmp[1024];
            u32 chunk = l > sizeof(tmp) ? sizeof(tmp) : l;
            if (!readn(tmp, chunk)) return 0;
            l -= chunk;
        }
    }
    return 1;
}

static int connect_server(const char *addr, int port) {
    WSADATA w;
    struct sockaddr_in a;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0) return 0;
    conn = socket(AF_INET, SOCK_STREAM, 0);
    if (conn == INVALID_SOCKET) return 0;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons((u_short)port);
    if (inet_pton(AF_INET, addr, &a.sin_addr) != 1) return 0;
    if (connect(conn, (void*)&a, sizeof(a)) != 0) return 0;
    return 1;
}

static int upload_block(const u8 *data, u32 len, H out) {
    u8 st = 1;
    u8 *resp = 0;
    u32 resp_len = 0;
    if (!request(2, data, len, &st, &resp, &resp_len)) return 0;
    if (st == 0 && resp && resp_len == 32) {
        memcpy(out, resp, 32);
        free(resp);
        return 1;
    }
    free(resp);
    return 0;
}

static int set_user_boot(const H user, const H boot_hash) {
    u8 body[96];
    u8 st = 1;
    memcpy(body, user, 32);
    memcpy(body + 32, BOOT_KEY, 32);
    memcpy(body + 64, boot_hash, 32);
    if (!request(7, body, sizeof(body), &st, 0, 0)) return 0;
    return st == 0;
}

static int add_edge(const H parent, const H child) {
    u8 body[64];
    u8 st = 1;
    memcpy(body, parent, 32);
    memcpy(body + 32, child, 32);
    if (!request(4, body, sizeof(body), &st, 0, 0)) return 0;
    return st == 0;
}

static int vote_edge(const H user, const H parent, const H child) {
    u8 body[96];
    u8 st = 1;
    memcpy(body, user, 32);
    memcpy(body + 32, parent, 32);
    memcpy(body + 64, child, 32);
    if (!request(6, body, sizeof(body), &st, 0, 0)) return 0;
    return st == 0;
}

static int map_token(const char *name, H out) {
    FILE *f = fopen("mods_map.txt", "rb");
    char line[512];
    size_t name_len = strlen(name);
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        char *tab = strchr(line, '\t');
        if (!tab) continue;
        if ((size_t)(tab - line) == name_len && memcmp(line, name, name_len) == 0) {
            char hex[65];
            if (strlen(tab + 1) < 64) { fclose(f); return 0; }
            memcpy(hex, tab + 1, 64);
            hex[64] = 0;
            int ok = hex_to_h(hex, out);
            fclose(f);
            return ok;
        }
    }
    fclose(f);
    return 0;
}

static int upload_cache_dir(const char *dir, int dry_run, u32 *uploaded, u32 *skipped) {
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        char path[MAX_PATH];
        H name_hash, actual_hash, uploaded_hash;
        u8 *data;
        u32 len = 0;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (!hex_to_h(fd.cFileName, name_hash)) { (*skipped)++; continue; }
        snprintf(path, sizeof(path), "%s\\%s", dir, fd.cFileName);
        data = read_file(path, &len);
        if (!data) { (*skipped)++; continue; }
        if (len == 0) { free(data); (*skipped)++; continue; }
        if (!sha256(data, len, actual_hash) || memcmp(name_hash, actual_hash, 32) != 0) {
            fprintf(stderr, "skip hash mismatch: %s\n", fd.cFileName);
            free(data);
            (*skipped)++;
            continue;
        }
        if (!dry_run) {
            if (!upload_block(data, len, uploaded_hash) || memcmp(uploaded_hash, actual_hash, 32) != 0) {
                fprintf(stderr, "upload failed: %s\n", fd.cFileName);
                free(data);
                FindClose(h);
                return 0;
            }
        }
        free(data);
        (*uploaded)++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return 1;
}

static void usage(void) {
    printf("usage: first_boot [--dry-run] [--hash <64hex>] [--cache <dir>] [--id <id.bin>] [--addr <ip>]\n");
}

int main(int argc, char **argv) {
    const char *boot_hex = DEFAULT_BOOT_HASH;
    const char *cache_dir = "cache";
    const char *id_path = "id.bin";
    const char *addr = DEFAULT_ADDR;
    int dry_run = 0;
    H user, boot_hash, root, boot_run;
    char boot_hex_out[65];
    u32 uploaded = 0, skipped = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dry-run") == 0) dry_run = 1;
        else if (strcmp(argv[i], "--hash") == 0 && i + 1 < argc) boot_hex = argv[++i];
        else if (strcmp(argv[i], "--cache") == 0 && i + 1 < argc) cache_dir = argv[++i];
        else if (strcmp(argv[i], "--id") == 0 && i + 1 < argc) id_path = argv[++i];
        else if (strcmp(argv[i], "--addr") == 0 && i + 1 < argc) addr = argv[++i];
        else { usage(); return 2; }
    }

    if (!hex_to_h(boot_hex, boot_hash)) { fprintf(stderr, "invalid boot hash\n"); return 1; }
    if (!read_hash_file(id_path, user)) { fprintf(stderr, "%s must exist and contain 32 bytes\n", id_path); return 1; }

    char boot_path[MAX_PATH];
    snprintf(boot_path, sizeof(boot_path), "%s\\%s", cache_dir, boot_hex);
    u32 boot_len = 0;
    u8 *boot_data = read_file(boot_path, &boot_len);
    if (!boot_data) { fprintf(stderr, "missing boot cache block: %s\n", boot_path); return 1; }
    free(boot_data);

    h_to_hex(boot_hash, boot_hex_out);
    printf("boot_hash\t%s\n", boot_hex_out);
    if (dry_run) printf("mode\tdry-run\n");

    if (!dry_run && !connect_server(addr, DEFAULT_PORT)) { fprintf(stderr, "connect failed: %s:%d\n", addr, DEFAULT_PORT); return 1; }
    if (!upload_cache_dir(cache_dir, dry_run, &uploaded, &skipped)) { fprintf(stderr, "cache upload failed\n"); return 1; }
    printf("cache_blocks\t%u\n", uploaded);
    printf("cache_skipped\t%u\n", skipped);

    if (!dry_run) {
        if (!set_user_boot(user, boot_hash)) { fprintf(stderr, "set CVM_BOOT failed\n"); return 1; }
        zero(root);
        if (map_token("boot_run", boot_run)) {
            if (!add_edge(root, boot_run)) fprintf(stderr, "warning: root->boot_run edge failed\n");
            if (!vote_edge(user, root, boot_run)) fprintf(stderr, "warning: root->boot_run vote failed\n");
        } else {
            fprintf(stderr, "warning: boot_run missing from mods_map.txt\n");
        }
    }

    printf("first_boot\tready\n");
    return 0;
}
