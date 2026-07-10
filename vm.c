#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib,"ws2_32.lib")

typedef unsigned char u8;
typedef unsigned u32;
typedef u8 H[32];
typedef void (*Fn)();

__declspec(dllexport) SOCKET conn;
__declspec(dllexport) Fn imp;

void rd(void *b,u32 n){for(u8*p=b;n;){int r=recv(conn,(char*)p,n,0);p+=r;n-=r;}}
void op(u8 o,void*b,u32 n){u8 h[5]={o,n>>24,n>>16,n>>8,n};send(conn,(char*)h,5,0);send(conn,b,n,0);}
u8 *rx(){u8 h[5];rd(h,5);u32 n=(u32)h[1]<<24|h[2]<<16|h[3]<<8|h[4];u8*b=malloc(n);rd(b,n);return b;}
u8 *download(H h){op(3,h,32);return rx();}

__declspec(dllexport) void cvm_firstchild(H p,H c){op(5,p,32);u8*b=rx();memcpy(c,b+4,32);free(b);}

/* find() is on the per-instruction hot path. Cache Fn by token so we do not
 * LoadLibraryA + GetProcAddress hundreds of times per frame. */
#define FIND_CACHE 256
typedef struct { H key; Fn fn; int on; } FindSlot;
static FindSlot find_slots[FIND_CACHE];

static u32 find_hash(const H h) {
    /* cheap mix of first/last bytes; table is sparse enough */
    return (u32)h[0] | ((u32)h[1] << 8) | ((u32)h[2] << 16) | ((u32)h[3] << 24);
}

__declspec(dllexport) Fn find(H h){
    u32 idx = find_hash(h) & (FIND_CACHE - 1);
    for (u32 n = 0; n < FIND_CACHE; n++) {
        u32 i = (idx + n) & (FIND_CACHE - 1);
        if (find_slots[i].on && !memcmp(find_slots[i].key, h, 32))
            return find_slots[i].fn;
        if (!find_slots[i].on) {
            char p[75] = "mods/";
            for (int k = 0; k < 32; k++) sprintf(p + 5 + k * 2, "%02x", h[k]);
            strcat(p, ".dll");
            HMODULE mod = LoadLibraryA(p);
            Fn f = mod ? (Fn)GetProcAddress(mod, "run") : 0;
            /* Cache negatives too so missing logical tokens skip filesystem. */
            memcpy(find_slots[i].key, h, 32);
            find_slots[i].fn = f;
            find_slots[i].on = 1;
            return f;
        }
    }
    /* table full: fall back without caching */
    {
        char p[75] = "mods/";
        for (int k = 0; k < 32; k++) sprintf(p + 5 + k * 2, "%02x", h[k]);
        strcat(p, ".dll");
        HMODULE mod = LoadLibraryA(p);
        return mod ? (Fn)GetProcAddress(mod, "run") : 0;
    }
}

int main(){WSADATA w;H h={0};struct sockaddr_in a={0};WSAStartup(0x202,&w);conn=socket(2,1,0);a.sin_family=2;a.sin_port=htons(9000);inet_pton(2,"118.25.42.70",&a.sin_addr);connect(conn,(void*)&a,sizeof a);cvm_firstchild(h,h);imp=find(*(H*)download(h));for(;;)imp();}
