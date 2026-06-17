// CVM — 极简虚拟机
// gcc vm.c -Os -s -o cvm.exe -lws2_32
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define CVM_SERVER "118.25.42.70"
#define CVM_PORT 9000
#define H 32
#define N 256
#define OP_FILE 3
#define OP_CHILDREN 5
typedef unsigned char u8;
typedef struct { u8 *p; uint32_t n; } Buf;
typedef void (*Op)(void);

// —— Host 接口 ——
typedef struct {
    void (*op)(u8 *, Op);
    void (*op_name)(char *, Op);
    void (*del)(u8 *);
    void (*del_name)(char *);
    void (*override)(u8 *, u8 *, DWORD);
    void (*touch)(void);
    Buf (*rpc)(uint8_t, u8 *, DWORD);
    void (*run)(u8 *);
    void (*enter)(u8 *);
    void (*adv)(void);
    void (*push)(u8 *, uint32_t);
    Buf (*pop)(void);
    Buf *(*top)(void);
    void *cur;
    u8 *pay; uint32_t plen;
    void (*next)(void);
    void (*next_noadv)(void);
} Host;

typedef struct { uint32_t off; Buf f; u8 key[H]; } Frame;
typedef struct { u8 id[H]; Op fn; } Ins;
typedef struct { u8 key[H]; Buf f; } Ov;

// —— 全局状态 ——
static SOCKET sock;
static Host host;
static Frame cur, ret[N];
static int active, rn;
static Ins ins[N]; int in;
static Ov ov[N]; int on;
static Buf st[N]; int sn;
static u8 ZERO[H];
void (*imp)(void);

// —— 前向声明 ——
static void cvm_step(void);
static void root(void);

// —— 工具 ——
static uint32_t U(u8 *p) { uint32_t x; memcpy(&x,p,4); return x; }
static int Z(u8 *p) { for (int i=0;i<H;i++) if(p[i]) return 0; return 1; }
static Buf B(u8 *p, DWORD n) { Buf b={malloc(n),n}; if(n) memcpy(b.p,p,n); return b; }
static void T(char *s, u8 o[H]) { DWORD n=strlen(s);memset(o,0,H);memcpy(o,s,n>H?H:n); }

// —— 网络 ——
static int xfer(u8 *p, int n, int rd) {
    while (n) { int k=rd?recv(sock,(char*)p,n,0):send(sock,(char*)p,n,0);
        if(k<=0){sock=INVALID_SOCKET;return 0;} p+=k;n-=k; }
    return 1;
}
Buf cvm_rpc(uint8_t op, u8 *body, DWORD len) {
    u8 h[5];Buf b={0};uint32_t be=htonl(len);
    h[0]=op;memcpy(h+1,&be,4);
    if(!xfer(h,5,0)||(len&&!xfer(body,len,0))||!xfer(h,5,1))return b;
    b.n=ntohl(*(uint32_t*)(h+1));b.p=malloc(b.n);
    if(b.n&&!xfer(b.p,b.n,1)){free(b.p);b.p=0;b.n=0;}
    return b;
}

// —— 栈 ——
void cvm_push(u8 *p, uint32_t n) { st[sn++]=B(p,n); }
Buf cvm_pop(void) { Buf z={0}; return sn?st[--sn]:z; }
Buf *cvm_top(void) { return sn?st+sn-1:0; }

// —— 指令表 ——
void cvm_op(u8 *id, Op fn) { memcpy(ins[in].id,id,H); ins[in++].fn=fn; }
void cvm_op_name(char *s, Op fn) { u8 id[H]; T(s,id); cvm_op(id,fn); }
void cvm_del(u8 *id) { memcpy(ins[in].id,id,H); ins[in++].fn=0; }
void cvm_del_name(char *s) { u8 id[H]; T(s,id); cvm_del(id); }
static Op opfind(u8 *id) { for(int i=in-1;i>=0;i--) if(!memcmp(ins[i].id,id,H)) return ins[i].fn; return 0; }

// —— OV 缓存 ——
static Ov *ovfind(u8 *k) { for(int i=on-1;i>=0;i--) if(!memcmp(ov[i].key,k,H)) return ov+i; return 0; }
void cvm_override(u8 *k, u8 *f, DWORD n) {
    Ov *o=ovfind(k); if(!o){o=ov+on++;memcpy(o->key,k,H);}
    free(o->f.p); o->f=B(f,n);
}
void cvm_touch(void) { cvm_override(cur.key,cur.f.p,cur.f.n); }

// —— 块获取 ——
static Buf getfile(u8 *k) {
    Ov *o=ovfind(k); if(o) return B(o->f.p,o->f.n);
    u8 h[H]={0}; Buf r=cvm_rpc(OP_CHILDREN,k,H);
    if(r.p&&r.n>=36) memcpy(h,r.p+4,H);
    free(r.p);
    return cvm_rpc(OP_FILE,h,H);
}

// —— 标准持续 ——
void cvm_adv(void) { cur.off += H + U(cur.f.p + cur.off + H); }

static void cvm_next(void) {
    if(active) cvm_adv();
    imp = cvm_step;
}
static void cvm_next_noadv(void) {
    imp = cvm_step;
}

// —— 帧管理 ——
void cvm_enter(u8 *k) {
    if(active) { cvm_adv(); ret[rn++] = cur; }
    cur.f = getfile(k);
    cur.off = 0;
    memcpy(cur.key, k, H);
    active = 1;
}
static void leave(void) {
    free(cur.f.p);
    if(rn) cur = ret[--rn];
    else active = 0;
}

// —— RUN ——
void cvm_run(u8 *h) {
    Op op = opfind(h);
    if(op) {
        host.pay = cur.f.p + cur.off + H + 4;
        host.plen = U(cur.f.p + cur.off + H) - 4;
        imp = op;
    } else { cvm_enter(h); }
}

// —— 步进 ——
static void cvm_step(void) {
    if(!active) { imp = root; return; }
    if(cur.off + H > cur.f.n || Z(cur.f.p + cur.off)) { leave(); return; }
    cvm_run(cur.f.p + cur.off);
}

// —— 根 ——
static void root(void) {
    cvm_enter(ZERO);
    imp = cvm_step;
}

// —— 加载 mods ——
void boot(void) {
    memset(ZERO,0,H);

    WSADATA w;struct sockaddr_in a={0};int to=5000;
    WSAStartup(MAKEWORD(2,2),&w);
    sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    a.sin_family=AF_INET;a.sin_port=htons(CVM_PORT);
    a.sin_addr.s_addr=inet_addr(CVM_SERVER);
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&to,sizeof to);
    setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(char*)&to,sizeof to);
    if(connect(sock,(struct sockaddr*)&a,sizeof a)==SOCKET_ERROR)
        {closesocket(sock);sock=INVALID_SOCKET;}

    host.op=cvm_op;host.op_name=cvm_op_name;host.del=cvm_del;host.del_name=cvm_del_name;
    host.override=cvm_override;host.touch=cvm_touch;host.rpc=cvm_rpc;
    host.run=cvm_run;host.enter=cvm_enter;host.adv=cvm_adv;
    host.push=cvm_push;host.pop=cvm_pop;host.top=cvm_top;
    host.cur=&cur;host.pay=0;host.plen=0;
    host.next=cvm_next;host.next_noadv=cvm_next_noadv;

    { HWND h=CreateWindowExA(0,"STATIC","",0,0,0,0,0,0,0,0,0); if(h) DestroyWindow(h); }

    WIN32_FIND_DATAA fd;char ms[N][MAX_PATH];int n=0;
    HANDLE hf=FindFirstFileA("mods\\*.dll",&fd);
    if(hf!=INVALID_HANDLE_VALUE){
        do wsprintfA(ms[n++],"mods\\%s",fd.cFileName);
        while(n<N&&FindNextFileA(hf,&fd));FindClose(hf);
        for(int i=0;i<n-1;i++)for(int j=i+1;j<n;j++)
            if(lstrcmpiA(ms[i],ms[j])>0){char t[MAX_PATH];lstrcpyA(t,ms[i]);lstrcpyA(ms[i],ms[j]);lstrcpyA(ms[j],t);}
        for(int i=0;i<n;i++){HMODULE m=LoadLibraryA(ms[i]);
            if(m){void(*init)(Host*);*(void**)&init=GetProcAddress(m,"cvm_init");
                if(init)init(&host);}}
    }

    imp = root;
    Sleep(100);
}

int main() {
    boot();
    while (1) { imp(); }
}
