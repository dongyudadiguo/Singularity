$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force mods | Out-Null

gcc .\cvm.c -Os -s -o .\cvm.exe -lws2_32

gcc -shared .\mods_src\00_core.c    -Os -s -o .\mods\00_core.dll    -lbcrypt
gcc -shared .\mods_src\01_graph.c   -Os -s -o .\mods\01_graph.dll
gcc -shared .\mods_src\02_block.c   -Os -s -o .\mods\02_block.dll
gcc -shared .\mods_src\03_runtime.c -Os -s -o .\mods\03_runtime.dll
gcc -shared .\mods_src\04_data.c    -Os -s -o .\mods\04_data.dll
gcc -shared .\mods_src\05_env.c     -Os -s -o .\mods\05_env.dll -lgdi32 -luser32

Write-Host "built cvm.exe"
Write-Host "built mods"