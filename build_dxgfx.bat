@echo off
g++ -shared dxgfx.cpp -o dxgfx.dll -ld2d1 -ldwrite -lole32 -lgdi32 -luser32 -Wl,--out-implib,libdxgfx.a
