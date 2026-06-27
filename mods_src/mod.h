#ifndef MOD_H
#define MOD_H

typedef unsigned char u8;
typedef unsigned u32;

extern __declspec(dllimport) u8 *ptr;
extern __declspec(dllimport) void cont(void);
extern __declspec(dllimport) void *pop(void);
extern __declspec(dllimport) void push(const void *p, u32 size);

#endif
