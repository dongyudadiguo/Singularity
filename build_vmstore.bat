@echo off
call build_vm.bat
gcc -shared vmstore.c -o vmstore.dll libvm.a -Wl,--out-implib,libvmstore.a -lws2_32
