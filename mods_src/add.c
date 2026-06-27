typedef unsigned char u8;
typedef unsigned u32;

extern u32 pop();
extern void push(u32 v, u32 n);
extern void cont();

void add() {
    push(pop() + pop(), 4);
    cont();
}
