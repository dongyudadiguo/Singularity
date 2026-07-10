@echo off

setlocal enabledelayedexpansion



call build_cont.bat

call build_vmstack.bat

call build_vmvar.bat

call build_vmstore.bat

call build_vmexec.bat

call build_dxgfx.bat

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

gcc -shared mods_src/block_payload_read.c -o mods/block_payload_read.dll libcont.a libvmstack.a libvmstate.a libvmstore.a

gcc -shared mods_src/block_replace_payload.c -o mods/block_replace_payload.dll libcont.a libvmstack.a libvmstate.a libvmstore.a

gcc -shared mods_src/block_create_child.c -o mods/block_create_child.dll libcont.a libvmstack.a libvmstate.a libvmstore.a libvm.a -lws2_32 -ladvapi32

gcc -shared mods_src/bootstrap.c -o mods/bootstrap.dll libvm.a libvmexec.a libvmstate.a libvmstore.a -lws2_32 -ladvapi32

gcc -shared mods_src/keyboard.c -o mods/keyboard.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a -luser32

gcc -shared mods_src/mouse.c -o mods/mouse.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a -luser32

gcc -shared mods_src/drawtext.c -o mods/drawtext.dll libcont.a libvmstate.a libdxgfx.a

gcc -shared mods_src/drawrect.c -o mods/drawrect.dll libcont.a libvmstate.a libdxgfx.a

gcc -shared mods_src/drawline.c -o mods/drawline.dll libcont.a libvmstate.a libdxgfx.a

gcc -shared mods_src/frame_begin.c -o mods/frame_begin.dll libcont.a libvmstate.a libdxgfx.a

gcc -shared mods_src/frame_clear.c -o mods/frame_clear.dll libcont.a libvmstate.a libdxgfx.a

gcc -shared mods_src/frame_end.c -o mods/frame_end.dll libcont.a libvmstate.a libdxgfx.a

gcc -shared mods_src/window_should_close.c -o mods/window_should_close.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a

gcc -shared mods_src/screen_size.c -o mods/screen_size.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a

gcc -shared mods_src/input_snapshot.c -o mods/input_snapshot.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a





gcc -shared mods_src/key_down.c -o mods/key_down.dll libcont.a libvmstack.a libvmstate.a -luser32

gcc -shared mods_src/key_pressed.c -o mods/key_pressed.dll libcont.a libvmstack.a libvmstate.a -luser32

gcc -shared mods_src/block_select_payload.c -o mods/block_select_payload.dll libcont.a libvmstate.a libvmstore.a libvm.a -lws2_32 -ladvapi32

gcc -shared mods_src/block_offset_at_index.c -o mods/block_offset_at_index.dll libcont.a libvmstack.a libvmstate.a libvmstore.a

gcc -shared mods_src/block_token_hex.c -o mods/block_token_hex.dll libcont.a libvmstack.a libvmstate.a libvmstore.a

gcc -shared mods_src/u32_hex.c -o mods/u32_hex.dll libcont.a libvmstack.a

gcc -shared mods_src/drawtext_stack.c -o mods/drawtext_stack.dll libcont.a libvmstack.a libvmstate.a libdxgfx.a

gcc -shared mods_src/jump_payload.c -o mods/jump_payload.dll libvmexec.a libvmstate.a libvmstore.a libvm.a -lws2_32 -ladvapi32

gcc -shared mods_src/block_token_name.c -o mods/block_token_name.dll libcont.a libvmstack.a libvmstate.a libvmstore.a

gcc -shared mods_src/block_payload_summary.c -o mods/block_payload_summary.dll libcont.a libvmstack.a libvmstate.a libvmstore.a

gcc -shared mods_src/text_input.c -o mods/text_input.dll libcont.a libvmstack.a libdxgfx.a

gcc -shared mods_src/string_append_var.c -o mods/string_append_var.dll libcont.a libvmstack.a libvmstate.a libvmvar.a

gcc -shared mods_src/string_backspace_var.c -o mods/string_backspace_var.dll libcont.a libvmstate.a libvmvar.a

gcc -shared mods_src/string_clear_var.c -o mods/string_clear_var.dll libcont.a libvmstate.a libvmvar.a

gcc -shared mods_src/registry_find.c -o mods/registry_find.dll libcont.a libvmstack.a

gcc -shared mods_src/block_insert_stack.c -o mods/block_insert_stack.dll libcont.a libvmstack.a libvmstore.a

gcc -shared mods_src/drawtext_var.c -o mods/drawtext_var.dll libcont.a libvmstate.a libvmvar.a libdxgfx.a

gcc -shared mods_src/registry_token_name.c -o mods/registry_token_name.dll libcont.a libvmstack.a

gcc -shared mods_src/u32_dec_sat.c -o mods/u32_dec_sat.dll libcont.a libvmstack.a

gcc -shared mods_src/camera_set.c -o mods/camera_set.dll libcont.a libvmstate.a libdxgfx.a

echo.

echo === Hash rename ===

if exist mod_tokens.txt del mod_tokens.txt

for %%f in (mods\*.dll) do call :hash_rename "%%f"

echo === Done ===

goto :eof



:hash_rename

for /f "skip=1 delims=" %%h in ('certutil -hashfile "%~1" SHA256') do set "hash=%%h" & goto :got_hash

:got_hash

set "hash=!hash: =!"
echo %~n1=!hash!>>mod_tokens.txt

if /i not "%~nx1"=="!hash!.dll" (

    if not exist "mods\!hash!.dll" (

        ren "%~1" "!hash!.dll"

        echo %~nx1 -^> !hash!.dll

    ) else (

        echo warn: !hash!.dll exists, skip %~nx1

    )

) else (

    echo skip %~nx1

)

goto :eof

