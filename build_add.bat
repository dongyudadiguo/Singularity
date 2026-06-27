gcc -shared mods_src\add.c -o mods\add.dll -Dadd=run -Wl,--unresolved-symbols=ignore-all
