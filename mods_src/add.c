typedef unsigned char u8;
typedef unsigned u32;

extern void* pop();
extern void push(void *, u32 size);
extern void cont();

void add() {
    /// push(pop() + pop(), 4); 伪代码
    cont();
}
