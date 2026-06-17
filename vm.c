#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

/* ============================================================
 *  你只需要看 main() 之上的"第3/4层"。
 *  上面两层是轮子，看不懂可以无视。
 * ============================================================ */

typedef unsigned char u8;
typedef unsigned int  u32;
typedef unsigned short u16;
typedef u8 Hash[32];          /* 一个 32 字节的哈希 */

/* ============================================================
 *  第1层：底层细节（网络字节搬运、字节序）—— 不用关心
 * ============================================================ */

static SOCKET g_conn;

static void net_must(int ok)      { if (!ok) exit(1); }
static void put_u32(u8 *p, u32 v) { p[0]=v>>24; p[1]=v>>16; p[2]=v>>8; p[3]=v; }
static u32  get_u32(const u8 *p)  { return (u32)p[0]<<24|p[1]<<16|p[2]<<8|p[3]; }

static void net_read(void *buf, u32 n) {
    u32 got = 0;
    while (got < n) {
        int r = recv(g_conn, (char*)buf + got, n - got, 0);
        net_must(r > 0);
        got += r;
    }
}

static void net_write(const void *buf, u32 n) {
    if (n) net_must(send(g_conn, (const char*)buf, n, 0) > 0);
}

static void net_connect(const char *ip, u16 port) {
    WSADATA w; WSAStartup(MAKEWORD(2,2), &w);
    g_conn = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &a.sin_addr);
    net_must(connect(g_conn, (struct sockaddr*)&a, sizeof(a)) == 0);
}

/* ============================================================
 *  第2层：协议层（一问一答）—— 一般不用改
 *  消息格式：[1字节操作码][4字节长度][数据...]
 * ============================================================ */

static void msg_send(u8 op, const void *body, u32 len) {
    u8 head[5];
    head[0] = op;
    put_u32(head + 1, len);
    net_write(head, 5);
    net_write(body, len);
}

static u8 *msg_recv(u32 *out_len) {
    u8 head[5];
    net_read(head, 5);
    u32 len = get_u32(head + 1);
    u8 *body = malloc(len + 1);
    net_read(body, len);
    body[len] = 0;
    if (out_len) *out_len = len;
    return body;
}

static void msg_call_ignore(u8 op, const void *body, u32 len) {
    msg_send(op, body, len);
    free(msg_recv(NULL));
}

/* ============================================================
 *  第3层：高层语义（★ 你主要看这里 ★）
 * ============================================================ */

/* 上传一段数据，服务器返回它的哈希 */
static void cvm_upload(const void *data, u32 len, Hash out_hash) {
    msg_send(2, data, len);
    u8 *reply = msg_recv(NULL);
    memcpy(out_hash, reply, 32);
    free(reply);
}

/* 按哈希取回文件内容；out_len 可为 NULL。返回内存需 free()。 */
static u8 *cvm_load_file(const Hash h, u32 *out_len) {
    msg_send(3, h, 32);
    return msg_recv(out_len);
}

/* 在 parent -> child 之间连一条边 */
static void cvm_link(const Hash parent, const Hash child) {
    u8 pair[64];
    memcpy(pair,      parent, 32);
    memcpy(pair + 32, child,  32);
    msg_call_ignore(4, pair, 64);
}

/* 取 parent 的第一个孩子，写入 out_child */
static void cvm_first_child(const Hash parent, Hash out_child) {
    msg_send(5, parent, 32);
    u8 *reply = msg_recv(NULL);
    memcpy(out_child, reply + 4, 32);   /* 跳过前4字节 */
    free(reply);
}

/* ============================================================
 *  第4层：业务逻辑（哈希 -> 处理函数 的表，沿孩子往下走）
 * ============================================================ */

typedef void (*Handler)(void);

static struct { Hash h; Handler fn; } g_table[256];
static int g_table_n;

static void handler_register(const Hash h, Handler fn) {
    if (g_table_n >= (int)(sizeof g_table / sizeof g_table[0])) return;
    memcpy(g_table[g_table_n].h, h, 32);
    g_table[g_table_n].fn = fn;
    g_table_n++;
}

static Handler handler_lookup(const Hash h) {
    for (int i = 0; i < g_table_n; i++)
        if (!memcmp(g_table[i].h, h, 32))
            return g_table[i].fn;
    return NULL;
}

/* 从 start 出发顺着"第一个孩子"往下找，落在某个已登记节点上 */
static Handler resolve_handler(Hash start) {
    Hash cur;
    memcpy(cur, start, 32);

    Handler fn;
    while (!(fn = handler_lookup(cur))) {
        Hash child, zero = {0};
        memset(child, 0, 32);
        cvm_first_child(cur, child);
        if (!memcmp(child, zero, 32)) {     /* 没孩子了 -> 找不到 */
            fprintf(stderr, "no handler found\n");
            exit(1);
        }
        memcpy(cur, child, 32);
    }
    memcpy(start, cur, 32);
    return fn;
}

/* ============================================================
 *  原 main 依赖的两个东西：全局 imp + boot()
 * ============================================================ */

static Handler imp;     /* main 会反复调用它 */

/* 连服务器，从根节点解析出处理函数，存进 imp */
static void boot(void) {
    net_connect("127.0.0.1", 9000);
    Hash root;
    memset(root, 0, 32);
    imp = resolve_handler(root);
}

/* ★ 不改动 ★ */
int main() { boot(); while (1) imp(); }