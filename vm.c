#include <stdio.h>      // 标准输入输出
#include <stdlib.h>     // malloc/exit 等
#include <string.h>     // memcpy/strcat 等
#include <winsock2.h>   // Windows 套接字
#include <ws2tcpip.h>   // inet_pton 等扩展
#include <windows.h>    // LoadLibrary 等 Win32 API
#pragma comment(lib, "ws2_32.lib")  // 链接 ws2_32 库

typedef unsigned char u8;  // 字节别名
typedef unsigned u32;      // 32 位无符号别名
typedef u8 H[32];          // 32 字节哈希类型

SOCKET conn;       // 全局连接套接字
H cur;             // 当前节点哈希
void (*imp)();     // 当前要执行的函数指针

void readn(void *b, u32 n) {                 // 精确读取 n 字节
    u32 g = 0;                               // 已读字节计数
    while (g < n) {                          // 未读满则继续
        int r = recv(conn, (char*)b+g, n-g, 0);  // 接收数据
        if (r < 1) exit(1);                  // 出错或断开则退出
        g += r;                              // 累加已读字节
    }
}

void send_op(u8 op, void *body, u32 len) {   // 发送一条带操作码的消息
    u8 h[5] = {op, len>>24, len>>16, len>>8, len};  // 1字节操作码+4字节大端长度
    send(conn, (char*)h, 5, 0);              // 发送头部
    if (len) send(conn, (char*)body, len, 0);  // 有内容则发送主体
}

u8 *recv_op() {                              // 接收一条消息并返回主体
    u8 h[5];                                 // 头部缓冲
    readn(h, 5);                             // 读取头部
    u32 l = (u32)h[1]<<24 | h[2]<<16 | h[3]<<8 | h[4];  // 解析大端长度
    u8 *b = malloc(l+1);                     // 分配主体缓冲（多 1 字节）
    readn(b, l);                             // 读取主体
    return b;                                // 返回主体（调用者负责释放）
}

void cvm_upload(void *d, u32 l, H out) { send_op(2, d, l); u8 *b = recv_op(); memcpy(out, b, 32); free(b); }  // 上传数据，返回其哈希
u8 *cvm_file(H h)          { send_op(3, h, 32); return recv_op(); }  // 按哈希取文件内容
void cvm_edge(H p, H c)    { u8 b[64]; memcpy(b,p,32); memcpy(b+32,c,32); send_op(4, b, 64); free(recv_op()); }  // 建立父子边
void cvm_firstchild(H p, H c) { send_op(5, p, 32); u8 *b = recv_op(); memcpy(c, b+4, 32); free(b); }  // 取父节点的第一个子节点哈希

typedef void (*Fn)();  // 无参无返回函数指针类型

Fn find(H h) {                                       // 根据哈希查找并加载对应模块
    char path[64] = "mods/";                         // 路径前缀
    for (int i = 0; i < 32; i++) sprintf(path+5+i*2, "%02x", h[i]);  // 哈希转十六进制文件名
    strcat(path, ".dll");                            // 追加扩展名
    HMODULE m = LoadLibraryA(path);                  // 加载 DLL
    return m ? (Fn)GetProcAddress(m, "run") : 0;     // 返回 run 导出函数，失败返回 0
}

void walk() {                                // 沿子节点遍历直到找到可加载模块
    Fn f;                                    // 找到的函数
    while (!(f = find(cur))) {               // 当前节点没有模块则继续
        H n;                                 // 下一节点哈希
        cvm_firstchild(cur, n);              // 取第一个子节点
        memcpy(cur, n, 32);                  // 移动到子节点
    }
    imp = f;                                 // 保存找到的函数
}

void boot() {                                        // 初始化与连接
    WSADATA w;                                       // Winsock 数据
    WSAStartup(MAKEWORD(2,2), &w);                   // 初始化 Winsock 2.2
    conn = socket(AF_INET, SOCK_STREAM, 0);          // 创建 TCP 套接字
    struct sockaddr_in a = {0};                      // 地址结构清零
    a.sin_family = AF_INET;                          // IPv4
    a.sin_port = htons(9000);                        // 端口 9000
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);    // 设置本地回环地址
    connect(conn, (void*)&a, sizeof(a));             // 连接服务器
    memset(cur, 0, 32);                              // 当前哈希清零（根节点）
    walk();                                          // 遍历找到首个模块
}

int main() { boot(); while (1) imp(); }  // 启动后无限循环执行当前模块