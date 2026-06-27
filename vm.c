#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")  

typedef unsigned char u8;
typedef u8 H[32];
typedef void (*Fn)();  

SOCKET conn;
H cur;
void (*imp)();

void readn(void *b, u32 n) {
    u32 g = 0;                               
    while (g < n) {                          
        int r = recv(conn, (char*)b+g, n-g, 0);  
        if (r < 1) exit(1);                  
        g += r;                              
    }
}

void send_op(u8 op, void *body, u32 len) {   
    u8 h[5] = {op, len>>24, len>>16, len>>8, len};  
    send(conn, (char*)h, 5, 0);              
    if (len) send(conn, (char*)body, len, 0);  
}

u8 *recv_op() {                              
    u8 h[5];                                 
    readn(h, 5);                             
    u32 l = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4];  
    u8 *b = malloc(l+1);                     
    readn(b, l);                             
    return b;                                
}

void cvm_firstchild(H p, H c) { send_op(5, p, 32); u8 *b = recv_op(); memcpy(c, b+4, 32); free(b); }  

Fn find(H h) {                                       
    char path[75] = "mods/";                         
    for (int i = 0; i < 32; i++) sprintf(path+5+i*2, "%02x", h[i]);  
    strcat(path, ".dll");                            
    HMODULE m = LoadLibraryA(path);                  
    return m ? (Fn)GetProcAddress(m, "run") : 0;     
}

void walk() {                                
    Fn f;                                    
    while (!(f = find(cur))) {               
        H n;                                 
        cvm_firstchild(cur, n);              
        memcpy(cur, n, 32);                  
    }
    imp = f;                                 
}

void boot()
{
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
    conn = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    inet_pton(AF_INET, "118.25.42.70", &a.sin_addr);
    connect(conn, (void *)&a, sizeof(a));
    memset(cur, 0, 32);
    walk();
}

int main() { boot(); while (1) imp(); }