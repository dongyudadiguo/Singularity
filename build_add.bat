gendef vm.exe
dlltool -d vm.def -l vm.lib
gcc -shared mods_src\add.c cont.c vm.lib -o mods\add.dll -Dadd=run -lws2_32
