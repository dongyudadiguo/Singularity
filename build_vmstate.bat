@echo off
gcc -shared vmstate.c -o vmstate.dll libvmstore.a -Wl,--out-implib,libvmstate.a
