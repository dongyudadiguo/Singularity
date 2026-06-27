@echo off
setlocal enabledelayedexpansion

call build_cont.bat
call build_vmstack.bat
gcc -shared mods_src/add.c -o mods/add.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/cond.c -o mods/cond.dll libcont.a libvmstack.a libvmexec.a libvmstate.a libvmstore.a libvm.a -lws2_32
gcc -shared mods_src/cond_payload.c -o mods/cond_payload.dll libcont.a libvmstack.a libvmexec.a libvmstate.a libvmstore.a libvm.a -lws2_32
gcc -shared mods_src/reexec.c -o mods/reexec.dll libvmexec.a libvmstate.a libvm.a
gcc -shared mods_src/cond_reexec.c -o mods/cond_reexec.dll libcont.a libvmstack.a libvmexec.a libvmstate.a libvmstore.a libvm.a -lws2_32

echo.
echo === 重命名 DLL 为 SHA-256 哈希名 ===
for %%f in (mods\*.dll) do call :hash_rename "%%f"
echo === 完成 ===
goto :eof

:hash_rename
for /f "skip=1 delims=" %%h in ('certutil -hashfile "%~1" SHA256') do set "hash=%%h" & goto :got_hash
:got_hash
set "hash=!hash: =!"
if /i not "%%~nx1"=="!hash!.dll" (
    if not exist "mods\!hash!.dll" (
        ren "%~1" "!hash!.dll"
        echo %%~nx1 -^> !hash!.dll
    ) else (
        echo 警告: !hash!.dll 已存在，跳过 %%~nx1
    )
) else (
    echo 跳过 %%~nx1 (已是哈希名^)
)
goto :eof