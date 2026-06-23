#define Package(name, body)\
void name (void){\
    body\
    imp = table[*(int*)(ptr += sizeof(int))];\
}
