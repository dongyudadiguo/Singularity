#include <string.h>
typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void *slot(u32);
extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
#include <windows.h>
static u8 previous[256];
__declspec(dllexport) void run(void){
    u32 vk=cvm_payload_size()>=4?*(u32*)cvm_payload():0;
    u8 down=vk<256&&(GetAsyncKeyState(vk)&0x8000)?1:0;
    u32 value=vk<256&&down&&!previous[vk]?1:0;
    if(vk<256)previous[vk]=down;
    memcpy(slot(4), &value, 4);
    cont();
}
