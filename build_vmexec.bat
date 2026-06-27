@echo off
call build_vm.bat
call build_vmstate.bat
call build_vmstore.bat
gcc -shared vmexec.c -o vmexec.dll libvm.a libvmstate.a libvmstore.a -Wl,--out-implib,libvmexec.a
