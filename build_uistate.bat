@echo off
setlocal
C:\mingw64\bin\gcc -shared uistate.c -o uistate.dll -Wl,--out-implib,libuistate.a
