@echo off
setlocal enabledelayedexpansion

call build_cont.bat
call build_vmstack.bat
call build_vmvar.bat
gcc -shared mods_src/add.c -o mods/add.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/cond.c -o mods/cond.dll libcont.a libvmstack.a libvmexec.a libvmstate.a libvmstore.a libvm.a -lws2_32
gcc -shared mods_src/cond_payload.c -o mods/cond_payload.dll libcont.a libvmstack.a libvmexec.a libvmstate.a libvmstore.a libvm.a -lws2_32
gcc -shared mods_src/reexec.c -o mods/reexec.dll libvmexec.a libvmstate.a libvm.a
gcc -shared mods_src/cond_reexec.c -o mods/cond_reexec.dll libcont.a libvmstack.a libvmexec.a libvmstate.a libvmstore.a libvm.a -lws2_32
gcc -shared mods_src/var_read.c -o mods/var_read.dll libcont.a libvmstack.a libvmvar.a libvmstate.a
gcc -shared mods_src/var_read_payload.c -o mods/var_read_payload.dll libcont.a libvmstack.a libvmvar.a libvmstate.a
gcc -shared mods_src/var_write.c -o mods/var_write.dll libcont.a libvmstack.a libvmvar.a libvmstate.a
gcc -shared mods_src/var_write_payload.c -o mods/var_write_payload.dll libcont.a libvmstack.a libvmvar.a libvmstate.a
gcc -shared mods_src/var_set.c -o mods/var_set.dll libcont.a libvmstack.a libvmvar.a libvmstate.a
gcc -shared mods_src/var_set_payload.c -o mods/var_set_payload.dll libcont.a libvmstack.a libvmvar.a libvmstate.a
gcc -shared mods_src/scope_start.c -o mods/scope_start.dll libcont.a libvmvar.a libvmstate.a
gcc -shared mods_src/scope_end.c -o mods/scope_end.dll libcont.a libvmvar.a libvmstate.a

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