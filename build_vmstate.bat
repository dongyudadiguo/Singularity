@echo off
gcc -shared vmstate.c -o vmstate.dll -Wl,--out-implib,libvmstate.a
