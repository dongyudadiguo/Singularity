#include "cvm_mod.h"

static Host *G;

static char *patharg(u8 *d, uint32_t n, Buf *b) {
    if (n) {
        Buf x = { d, n };
        return cstr(x);
    }

    *b = G->pop();
    return cstr(*b);
}

static void push32(uint32_t x) {
    u8 o[4];
    wr32(o, x);
    G->push(o, 4);
}

static int fs_read(u8 *d, uint32_t n) {
    Buf pb = {0}, out;
    char *p = patharg(d, n, &pb);
    DWORD got;

    HANDLE f = CreateFileA(p, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    out = mbuf(GetFileSize(f, 0));

    ReadFile(f, out.p, out.n, &got, 0);
    CloseHandle(f);

    G->push(out.p, out.n);

    free(out.p);
    free(p);
    free(pb.p);
    return 0;
}

static int fs_write(u8 *d, uint32_t n) {
    char *p;
    u8 *data;
    DWORD pn, dn, wrote;
    Buf path = {0}, body = {0};

    if (n) {
        pn = rd32(d);
        p = malloc(pn + 1);
        memcpy(p, d + 4, pn);
        p[pn] = 0;
        data = d + 4 + pn;
        dn = n - 4 - pn;
    } else {
        body = G->pop();
        path = G->pop();
        p = cstr(path);
        data = body.p;
        dn = body.n;
    }

    HANDLE f = CreateFileA(p, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    WriteFile(f, data, dn, &wrote, 0);
    CloseHandle(f);

    free(p);
    free(path.p);
    free(body.p);

    return 0;
}

static int fs_exists(u8 *d, uint32_t n) {
    Buf pb = {0};
    char *p = patharg(d, n, &pb);

    push32(GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES);

    free(p);
    free(pb.p);
    return 0;
}

static int fs_list(u8 *d, uint32_t n) {
    Buf pb = {0}, out = {0};
    char *p = patharg(d, n, &pb);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(p, &fd);

    do {
        DWORD l = strlen(fd.cFileName);
        out.p = realloc(out.p, out.n + l + 1);
        memcpy(out.p + out.n, fd.cFileName, l);
        out.n += l;
        out.p[out.n++] = '\n';
    } while (FindNextFileA(h, &fd));

    FindClose(h);

    G->push(out.p, out.n);

    free(out.p);
    free(p);
    free(pb.p);
    return 0;
}

static int fs_cwd(u8 *d, uint32_t n) {
    char p[MAX_PATH];
    DWORD l = GetCurrentDirectoryA(MAX_PATH, p);
    G->push((u8 *)p, l);
    return 0;
}

__declspec(dllexport)
void cvm_init(Host *h) {
    G = h;

    h->op_name("CVM1:FS:READ", fs_read);
    h->op_name("CVM1:FS:WRITE", fs_write);
    h->op_name("CVM1:FS:EXISTS", fs_exists);
    h->op_name("CVM1:FS:LIST", fs_list);
    h->op_name("CVM1:FS:CWD", fs_cwd);
}
