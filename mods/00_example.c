#include <windows.h>
#include <stdint.h>

#define H 32

typedef unsigned char u8;
typedef struct { u8 *p; DWORD n; } Buf;
typedef struct { Buf f; DWORD off; u8 key[H]; } Frame;
typedef int (*Op)(u8 *data, uint32_t len);

typedef struct Host {
    void (*op)(u8 *id, Op fn);
    void (*op_name)(char *name, Op fn);
    void (*del)(u8 *id);
    void (*del_name)(char *name);

    void (*override)(u8 *key, u8 *file, DWORD len);
    void (*touch)();

    Buf  (*post)(wchar_t *path, u8 *body, DWORD len);

    void (*run)(u8 *hash);
    void (*enter)(u8 *hash);
    void (*adv)();

    Frame *cur;
} Host;

/*
    指令从这里往下写。
    这里只占位，不预设任何指令。

    static int your_op(u8 *data, uint32_t len) {
        return 0; // 返回 0：VM 自动执行标准持续逻辑
    }
*/

__declspec(dllexport)
void cvm_init(Host *h) {
    // h->op_name("CVM1:YOUR:OP", your_op);
    // h->del_name("CVM1:OLD:OP");
}
