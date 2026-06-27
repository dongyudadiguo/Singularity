@echo off
call build_vmstate.bat
gcc -shared vmvar.c -o vmvar.dll libvmstate.a -Wl,--out-implib,libvmvar.a