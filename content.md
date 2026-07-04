## system ---

Proactively use Python for any task; cautious by default.
---

## user ----

继续处理 C:\Users\12159\Desktop\Singularity。

禁止读取：
- Singularity/content.md
- Singularity/agent
- Singularity/.git

约束：
- vm.c 不能修改。
- vmexec.c、vmstore.c、vmstate.c 一般不变，除非修 bug。
- 首运行程序必须由多个现有/新增 mod 组成，不是单个 mod。
- 函数/块按新版 logical block key[32] 表示。
- 指令 registry 从网络遍历：从 hash("#TAG") 开始，每个 child 的文件内容以 # 开头表示标签，否则 child hash 是指令 token。
- 底层以外，界面体验尽量接近旧版 transition/main.c 的 to_dest_dev_base，但不能直接照搬旧架构。

上一轮已完成：
- 扩展 dxgfx.cpp / dxgfx.h：帧循环、clear、输入快照、鼠标滚轮、窗口关闭、屏幕尺寸、camera/world mouse。
- 扩展 vmstore.c：cvm_children、cvm_file_read、cvm_sha256、cvm_edge。
- 新增 mod：
  - editor_init.c
  - editor_frame.c
  - frame_begin.c
  - frame_clear.c
  - frame_end.c
  - input_snapshot.c
  - screen_size.c
  - window_should_close.c
  - block_payload_read.c
  - block_replace_payload.c
  - block_create_child.c
- 恢复并更新 build_mods.bat，构建通过。
- 生成 first_full_editor_block.bin，内容是 editor_init -> editor_frame -> reexec -> zero terminator。
- 已上传首运行 editor block 到服务器并挂到 bootstrap child：
  - editor block hash: bed0ab62add7065803465a27b2d5793f1d93c6a15ed56292913959992da6e5d9
  - bootstrap token: 46e3a50739f8438f9da55bed965c9448b8074cad3f11436981892b92800db6ed
  - 查询确认 bootstrap first child 已是该 editor block。
- vm.exe 启动 5 秒未立即退出。

下一步建议：
- 实际打开 vm.exe 人工检查 editor 交互。
- 修 editor_frame.c 的体验细节和 bug。
- 检查 registry 是否已有 #TAG 图数据；如果没有，需要新增/上传 registry 标签和指令名文件，并连到 hash("#TAG")。