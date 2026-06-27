@echo off
if not exist libcont.a call build_cont.bat
gcc -shared vmstack.c -o vmstack.dll libcont.a -Wl,--out-implib,libvmstack.a
