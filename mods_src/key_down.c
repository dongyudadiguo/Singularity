typedef unsigned char u8; typedef unsigned u32;
extern __declspec(dllimport) void cont(void); extern __declspec(dllimport) void push(const void*,u32);
extern __declspec(dllimport) u8*cvm_payload(void); extern __declspec(dllimport) u32 cvm_payload_size(void);
#include <windows.h>
__declspec(dllexport) void run(void){u32 vk=cvm_payload_size()>=4?*(u32*)cvm_payload():0;u32 v=vk<256&&(GetAsyncKeyState(vk)&0x8000)?1:0;push(&v,4);cont();}
