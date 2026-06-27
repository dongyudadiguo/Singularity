@echo off
call build_vmstate.bat
gcc -shared vmstack.c -o vmstack.dll libvmstate.a -Wl,--out-implib,libvmstack.a
