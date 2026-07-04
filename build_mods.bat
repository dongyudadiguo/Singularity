@echo off
setlocal enabledelayedexpansion

call build_cont.bat
call build_vmstack.bat
call build_vmvar.bat
call build_dxgfx.bat
gcc -shared editorcore.c -o editorcore.dll -Wl,--out-implib,libeditorcore.a libdxgfx.a -lws2_32 -ladvapi32 -luser32
gcc -shared mods_src/add.c -o mods/add.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/ret.c -o mods/ret.dll libcont.a libvmstate.a
gcc -shared mods_src/halt.c -o mods/halt.dll
gcc -shared mods_src/sub.c -o mods/sub.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/mul.c -o mods/mul.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/div.c -o mods/div.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/mod.c -o mods/mod.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/eq.c -o mods/eq.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/neq.c -o mods/neq.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/gt.c -o mods/gt.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/lt.c -o mods/lt.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/gte.c -o mods/gte.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/lte.c -o mods/lte.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/and.c -o mods/and.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/or.c -o mods/or.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/not.c -o mods/not.dll libcont.a libvmstack.a libvmstate.a
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
gcc -shared mods_src/const_payload.c -o mods/const_payload.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/key_get.c -o mods/key_get.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/mouse_x.c -o mods/mouse_x.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/mouse_y.c -o mods/mouse_y.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/mouse_buttons.c -o mods/mouse_buttons.dll libcont.a libvmstack.a libvmstate.a
gcc -shared mods_src/block_len.c -o mods/block_len.dll libcont.a libvmstack.a libvmstate.a libvmstore.a
gcc -shared mods_src/block_read_token.c -o mods/block_read_token.dll libcont.a libvmstack.a libvmstate.a libvmstore.a
gcc -shared mods_src/block_insert_payload.c -o mods/block_insert_payload.dll libcont.a libvmstack.a libvmstate.a libvmstore.a libvmexec.a libvm.a -lws2_32 -ladvapi32
gcc -shared mods_src/block_delete.c -o mods/block_delete.dll libcont.a libvmstack.a libvmstate.a libvmstore.a
gcc -shared mods_src/block_flush.c -o mods/block_flush.dll libcont.a libvmstate.a libvmstore.a libvm.a -lws2_32 -ladvapi32
gcc -shared mods_src/bootstrap.c -o mods/bootstrap.dll libvm.a libvmexec.a libvmstate.a libvmstore.a -lws2_32 -ladvapi32

gcc -shared mods_src/keyboard.c -o mods/keyboard.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a -luser32
gcc -shared mods_src/mouse.c -o mods/mouse.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a -luser32
gcc -shared mods_src/drawtext.c -o mods/drawtext.dll libcont.a libvmstate.a libdxgfx.a
gcc -shared mods_src/drawrect.c -o mods/drawrect.dll libcont.a libvmstate.a libdxgfx.a
gcc -shared mods_src/drawline.c -o mods/drawline.dll libcont.a libvmstate.a libdxgfx.a

for %%m in (gfx_frame_begin gfx_clear gfx_frame_end gfx_screen_size gfx_window_should_close gfx_set_camera gfx_world_mouse key_down key_pressed key_released text_input mouse_wheel mouse_down) do gcc -shared mods_src/%%m.c -o mods/%%m.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a -luser32
for %%m in (editor_init editor_update_input editor_render_views editor_flush_current editor_frame editor_state_read editor_state_write editor_update_mouse editor_insert_auto editor_insert_block editor_insert_data editor_delete_range editor_copy_range editor_paste_range editor_move_cursor editor_render_completion editor_should_halt) do gcc -shared mods_src/%%m.c -o mods/%%m.dll libcont.a libvmstack.a libvmstate.a libeditorcore.a libdxgfx.a -luser32
for %%m in (block_next_offset block_prev_offset block_payload_read block_replace_payload block_payload_write block_copy_range block_move_range block_ensure_ret) do gcc -shared mods_src/%%m.c -o mods/%%m.dll libcont.a libvmstack.a libvmstate.a libvmstore.a

echo.
echo === 重命� DLL � SHA-256 哈希� ===
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
        echo 警告: !hash!.dll 已存�，跳� %%~nx1
    )
) else (
    echo 跳过 %%~nx1 (已是哈希名^)
)
goto :eof
