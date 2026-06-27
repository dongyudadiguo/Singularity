@echo off
if not exist libvm.a call build_vm.bat
gcc -shared cont.c -o cont.dll libvm.a -Wl,--out-implib,libcont.a -lws2_32
