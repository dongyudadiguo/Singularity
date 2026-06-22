#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <winsock2.h>   
#include <ws2tcpip.h>   
#include <windows.h>    
#pragma comment(lib, "ws2_32.lib")  

#define CVM_TYPES_DEFINED
typedef unsigned char u8;  
typedef unsigned u32;      
typedef unsigned long long u64;
typedef u8 H[32];          

#include "continue.h"

static const H BOOT_KEY_VM = {0x43,0x56,0x4d,0x5f,0x42,0x4f,0x4f,0x54};

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

u8 *recv_resp(u8 *status, u32 *out_len) {
    u8 h[5];                                 
    readn(h, 5);                             
    if (status) *status = h[0];
    u32 l = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4];  
    if (out_len) *out_len = l;
    u8 *b = malloc(l+1);                     
    readn(b, l);                             
    b[l] = 0;
    return b;                                
}

u8 *recv_op() { return recv_resp(0, 0); }

void cvm_upload(void *d, u32 l, H out) { send_op(2, d, l); u8 *b = recv_op(); memcpy(out, b, 32); free(b); }  
u8 *cvm_file_len(H h, u32 *out_len) {
    u8 st = 1;
    send_op(3, h, 32);
    u8 *b = recv_resp(&st, out_len);
    if (st != 0) { free(b); if (out_len) *out_len = 0; return 0; }
    return b;
}
u8 *cvm_file(H h)          { return cvm_file_len(h, 0); }
void cvm_edge(H p, H c)    { u8 b[64]; memcpy(b,p,32); memcpy(b+32,c,32); send_op(4, b, 64); free(recv_op()); }  
void cvm_firstchild(H p, H c) { send_op(5, p, 32); u8 *b = recv_op(); memcpy(c, b+4, 32); free(b); }  

int cvm_user(H out) {
    FILE *f = fopen("id.bin", "rb");
    if (!f) { cvm_zero(out); return 0; }
    size_t n = fread(out, 1, 32, f);
    fclose(f);
    if (n != 32) { cvm_zero(out); return 0; }
    return 1;
}

int cvm_uget(const u8 key[32], H out) {
    H user;
    u8 body[64];
    u8 st = 1;
    u32 len = 0;
    if (!cvm_user(user)) return 0;
    memcpy(body, user, 32);
    memcpy(body + 32, key, 32);
    send_op(8, body, 64);
    u8 *b = recv_resp(&st, &len);
    if (st == 0 && b && len == 32) { memcpy(out, b, 32); free(b); return 1; }
    free(b);
    return 0;
}

typedef void (*Fn)();  

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

int boot_connect() {
    WSADATA w;                                       
    if (WSAStartup(MAKEWORD(2,2), &w) != 0) return 0;
    conn = socket(AF_INET, SOCK_STREAM, 0);          
    if (conn == INVALID_SOCKET) return 0;
    struct sockaddr_in a = {0};                      
    a.sin_family = AF_INET;                          
    a.sin_port = htons(9000);                        
    inet_pton(AF_INET, "118.25.42.70", &a.sin_addr);    
    if (connect(conn, (void*)&a, sizeof(a)) != 0) return 0;
    return 1;
}

int boot() {
    H target;
    u32 len = 0;
    u8 *p;
    CvmState *s;

    if (!boot_connect()) return 0;
    cvm_zero(target);
    if (!cvm_uget(BOOT_KEY_VM, target)) return 0;

    s = cvm_state();
    if (s) {
        memcpy(s->view_hash, target, 32);
        memcpy(s->cur_hash, target, 32);
        s->ret_jb = 0;
    }

    p = cvm_file_len(target, &len);
    if (!p || !len) { free(p); return 0; }
    cbegin(p, len);
    free(p);
    return 1;
}

int main() { return boot() ? 0 : 1; }
