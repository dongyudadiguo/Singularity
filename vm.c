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

// —— VM 全局（Host.cur 指向这里）——
static struct {
    uint32_t off; Buf f; u8 key[H];  // 帧（03_runtime 通过 (Frame*)h->cur 访问）
    Buf st[N]; int sn;               // 数据栈
    struct { u8 id[H]; Op fn; } ins[N]; int in; // 指令表
    struct { u8 k[H]; Buf f; } ov[N]; int on;   // OV 缓存
} vm;

static SOCKET sock;
void (*imp)(void);

// —— Host 接口（mods 通过 h->pay/plen 获取当前 payload）——
typedef struct {
    void (*op)(u8 *, Op);
    void (*op_name)(char *, Op);
    void (*del)(u8 *);
    void (*del_name)(char *);
    void (*override)(u8 *, u8 *, DWORD);
    void (*touch)(void);
    Buf (*rpc)(uint8_t, u8 *, DWORD);
    void *run, *enter;
    void (*adv)(void);
    void (*push)(u8 *, uint32_t);
    Buf (*pop)(void);
    Buf *(*top)(void);
    void *cur;
    u8 *pay; uint32_t plen;
} Host;

// —— 工具 ——
static uint32_t U(u8 *p) { uint32_t x; memcpy(&x,p,4); return x; }
static int Z(u8 *p) { for (int i=0;i<H;i++) if(p[i]) return 0; return 1; }
static Buf B(u8 *p, DWORD n) { Buf b={malloc(n),n}; if(n) memcpy(b.p,p,n); return b; }

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

void cvm_push(u8 *p, uint32_t n) { vm.st[vm.sn++]=B(p,n); }
Buf cvm_pop(void) { Buf z={0}; return vm.sn?vm.st[--vm.sn]:z; }
Buf *cvm_top(void) { return vm.sn?vm.st+vm.sn-1:0; }

void cvm_op(u8 *id, Op fn) { memcpy(vm.ins[vm.in].id,id,H); vm.ins[vm.in++].fn=fn; }
void cvm_op_name(char *s, Op fn) {
    u8 id[H]; DWORD n=strlen(s); memset(id,0,H); memcpy(id,s,n>H?H:n); cvm_op(id,fn); }
void cvm_del(u8 *id) { memcpy(vm.ins[vm.in].id,id,H); vm.ins[vm.in++].fn=0; }
void cvm_del_name(char *s) {
    u8 id[H]; DWORD n=strlen(s); memset(id,0,H); memcpy(id,s,n>H?H:n); cvm_del(id); }

static Op opfind(u8 *id) {
    for(int i=vm.in-1;i>=0;i--) if(!memcmp(vm.ins[i].id,id,H)) return vm.ins[i].fn;
    return 0;
}

static void *ovfind(u8 *k) {
    for(int i=vm.on-1;i>=0;i--) if(!memcmp(vm.ov[i].k,k,H)) return vm.ov+i;
    return 0;
}
void cvm_override(u8 *k, u8 *f, DWORD n) {
    void *o=ovfind(k);
    if(!o){o=vm.ov+vm.on++;memcpy(o,k,H);}
    free(((Buf*)((u8*)o+H))->p);
    *((Buf*)((u8*)o+H))=B(f,n);
}
void cvm_touch(void) { cvm_override(vm.key,vm.f.p,vm.f.n); }

// —— 块获取：OV缓存 > 服务器 CHILDREN[0]→FILE ——
static Buf getfile(u8 *k) {
    void *o=ovfind(k);
    if(o) return B(((Buf*)((u8*)o+H))->p,((Buf*)((u8*)o+H))->n);
    u8 h[H]={0}; Buf r=cvm_rpc(OP_CHILDREN,k,H);
    if(r.p&&r.n>=36) memcpy(h,r.p+4,H); free(r.p);
    return cvm_rpc(OP_FILE,h,H);
}

// —— resolve：当前32字节匹配指令→返回；否则getfirstchild→递归 ——
static Op resolve(u8 *tok) {
    Op fn=opfind(tok); if(fn)return fn;
    Buf child=getfile(tok);
    if(!child.p||child.n<H){free(child.p);return 0;}
    fn=opfind(child.p); free(child.p); return fn;
}

// —— 标准持续：ptr += 32 + *(int*)ptr ——
static void adv(void) { vm.off+=H+U(vm.f.p+vm.off+H); }

// —— boot ——
void boot(void) {
    u8 ZERO[H]={0};
    WSADATA w;struct sockaddr_in a={0};int to=5000;
    WSAStartup(MAKEWORD(2,2),&w);
    sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    a.sin_family=AF_INET;a.sin_port=htons(CVM_PORT);
    a.sin_addr.s_addr=inet_addr(CVM_SERVER);
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&to,sizeof to);
    setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(char*)&to,sizeof to);
    if(connect(sock,(struct sockaddr*)&a,sizeof a)==SOCKET_ERROR)
        {closesocket(sock);sock=INVALID_SOCKET;}

    // 加载 mods
    WIN32_FIND_DATAA fd;char ms[N][MAX_PATH];int n=0;
    HANDLE hf=FindFirstFileA("mods\\*.dll",&fd);
    if(hf!=INVALID_HANDLE_VALUE){
        do wsprintfA(ms[n++],"mods\\%s",fd.cFileName);
        while(n<N&&FindNextFileA(hf,&fd));FindClose(hf);
        for(int i=0;i<n-1;i++)for(int j=i+1;j<n;j++)
            if(lstrcmpiA(ms[i],ms[j])>0){char t[MAX_PATH];lstrcpyA(t,ms[i]);lstrcpyA(ms[i],ms[j]);lstrcpyA(ms[j],t);}
        Host hh;
        hh.op=cvm_op;hh.op_name=cvm_op_name;hh.del=cvm_del;hh.del_name=cvm_del_name;
        hh.override=cvm_override;hh.touch=cvm_touch;hh.rpc=cvm_rpc;
        hh.run=0;hh.enter=0;hh.adv=adv;
        hh.push=cvm_push;hh.pop=cvm_pop;hh.top=cvm_top;hh.cur=&vm;
        hh.pay=0;hh.plen=0;
        for(int i=0;i<n;i++){HMODULE m=LoadLibraryA(ms[i]);
            if(m){void(*init)(Host*);*(void**)&init=GetProcAddress(m,"cvm_init");
                if(init)init(&hh);}}
    }

    vm.f=getfile(ZERO);memcpy(vm.key,ZERO,H);vm.off=0;
    imp=resolve(vm.f.p);
}

int main()
{
boot();
    while (1)
    {
        if(imp){imp();} // imp = 当前指令函数，op 内部负责推进或跳转
    }
}