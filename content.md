## system

主动调动 Python 工具完成任意任务，默认谨慎

## user

项目路径：C:\Users\12159\Desktop\Singularity
旧版本路径：C:\Users\12159\Desktop\transition
服务器源码镜像：C:\Users\12159\Desktop\server
服务器 IP：118.25.42.70
Singularity/id.bin 是已验证 id。

禁止读取：
- Singularity/content.md
- Singularity/agent
- Singularity/.git

限制：
- vm.c 不能修改。
- vmexec.c、vmstore.c、vmstate.c 一般不变，但可调整/修 bug。
- 首运行程序必须由现有/新增 mod 组成，不是单个 mod。
- 新建 mod 已被允许。
- 允许新增 editor/gfx/input/block 系列 mod。
- 允许修改 dxgfx.cpp / dxgfx.h。
- 函数/块按新版 logical block key[32] 表示，不用旧版内存地址。
- 指令名 registry 采用 A：registry block 数据化。

新版 block 格式：
token[32] + payload_size[u32] + payload[payload_size]
...
32 字节全零 token 作为结尾标记，不会被执行。

目标：
直接做完整版新版 Singularity 首运行程序，参考旧版 transition/main.c 的 to_dest_dev_base，但新版架构大改，不能直接照搬。遇到新的不兼容点必须先问我确认。

当前已做过的内容：
- 曾做过验证版和最小可编辑器，但还不是完整版。
- 已新增/调整过一些 mod 思路：const_payload、key_get、mouse_x、mouse_y、mouse_buttons、block_insert_payload、block_delete、block_flush、block_len、block_read_token。
- 变量系列 mod 需要支持任意大小任意数据。
- vmstack 原先用 ptr 做 stack，会和指令流冲突，应该使用独立 stack。
- reexec 原先会重复进入当前 block 压 frame，应该原地重启当前 block。
- block 编辑需要通过 cache 修改 + upload + user override 写回服务器。

完整版方向：
1. 扩展 dxgfx，支持完整帧循环、清屏、输入事件、鼠标滚轮、窗口关闭、屏幕尺寸、world mouse/camera。
2. 新增 editor/gfx/input/block 系列 mod，不做单个巨型 mod。
3. 做 registry block：name -> token[32]。
4. 首运行程序由多个 block/mod 组成，实现类似旧版 to_dest_dev_base：
   - editor init
   - 多 view
   - block 指令列表渲染
   - 鼠标选择插入点
   - 键盘输入和补全
   - 插入指令/data/block
   - 删除/复制/粘贴
   - 拖动 view、camera 移动、滚轮缩放
   - 修改后写回服务器
   - reexec 持续刷新

## user

以下是上次对话中你最后的回复：

可以直接做完整版，但这里有几个旧版到新版的**不兼容点**，按你的规则我必须先问你确认，不能直接继续硬改。

## 完整版需要新增/调整的能力

旧版 `to_dest_dev_base` 依赖 Raylib 和旧指令内存模型，新版目前不具备完整等价能力。要做完整版，我建议按“多 mod 组成的完整编辑器程序”实现，而不是单个巨型 mod。

### 需要新增一批 editor / gfx / input / block mod

我建议新增这些 mod 组：

#### 1. 图形帧控制 mod

新版 `drawtext/drawrect/drawline` 目前是每次 draw 自己 begin/end，不适合完整编辑器每帧清屏和批量绘制。

需要新增：

- `gfx_frame_begin`
- `gfx_clear`
- `gfx_frame_end`
- `gfx_screen_size`
- `gfx_window_should_close`
- `gfx_set_camera`
- `gfx_world_mouse`

同时可能需要调整 `dxgfx.cpp`，让它支持：

- 每帧 begin / clear / end；
- 鼠标滚轮；
- client 坐标；
- 窗口关闭状态；
- 可选 camera transform。

#### 2. 输入事件 mod

旧版使用：

- `IsKeyPressed`
- `IsKeyReleased`
- `IsKeyDown`
- `GetMouseWheelMove`
- 文本输入

新版目前只有 `keyboard` 当前状态快照，无法区分 pressed/released，也没有文本输入。

需要新增：

- `key_down`
- `key_pressed`
- `key_released`
- `text_input`
- `mouse_client`
- `mouse_world`
- `mouse_wheel`
- `mouse_down`
- `mouse_pressed`
- `mouse_released`

#### 3. 任意数据/字符串/列表 mod

旧版有大量 C 层字符串和数组操作，例如：

- `input_str`
- `completion`
- `remove_underscores`
- `find_index`
- 指令名表
- 函数列表
- view 列表

需要新增：

- `bytes_eq`
- `bytes_copy`
- `bytes_len`
- `bytes_append`
- `bytes_clear`
- `str_find_token_name`
- `str_remove_underscores`
- `str_is_digit`
- `u32_to_text`
- `hash_to_hex_text`

或者更直接一点，新增 editor 专用状态 mod。

#### 4. block 编辑 mod

目前已有：

- `block_insert_payload`
- `block_delete`
- `block_flush`
- `block_len`
- `block_read_token`

但完整版还需要：

- `block_replace_payload`
- `block_move_range`
- `block_copy_range`
- `block_find_instruction_at_y`
- `block_next_offset`
- `block_prev_offset`
- `block_payload_read`
- `block_payload_write`
- `block_ensure_ret`
- `block_create_child`
- `block_link_child`
- `block_resolve_child`
- `block_set_user_override`

#### 5. editor 状态 mod

旧版有很多全局状态：

- `base`
- `point`
- `copy`
- `view`
- `views`
- `views_pos`
- `camera`
- `input_str`
- `completion`
- `funcs`
- `fun_max`
- `var_*`
- `repeat_*`
- `debug_*`

如果全部用现有 `var_*` 拼出来，会非常复杂且性能差。完整版建议新增 editor 状态 mod，但仍然拆分，不做单个巨型 mod：

- `editor_init`
- `editor_state_read`
- `editor_state_write`
- `editor_update_input`
- `editor_update_mouse`
- `editor_insert_auto`
- `editor_insert_block`
- `editor_insert_data`
- `editor_delete_range`
- `editor_copy_range`
- `editor_paste_range`
- `editor_move_cursor`
- `editor_render_views`
- `editor_render_completion`
- `editor_flush_current`

这些 mod 合起来组成首运行程序，不是单个 mod。

---

## 需要你确认的关键不兼容设计

### 不兼容点 1：旧版函数指针 `funcs[]` 不能照搬

旧版里面函数/块是内存地址：

```c
funcs[fun_max++] = base = ptr;
```

新版 block 是：

```text
token[32] + payload_size + payload
```

并且通过服务器 hash / child / user override 解析。

所以完整版里“函数”我建议改成：

```text
function/block = logical block key[32]
```

也就是编辑器管理的是 block key，不再是裸内存指针。

请确认是否接受这个新版语义。

### 不兼容点 2：旧版直接 memmove 整个 base，新版必须通过 block cache + user override 写回

旧版：

```c
memmove(...)
change_ret(...)
```

新版不能直接依赖 hash 内容不变。编辑后必须：

1. 修改当前 block cache；
2. 上传新 block；
3. 对当前 logical key 设置 user override；
4. 后续执行 logical key 时解析到新 hash。

请确认完整版按这个方式做。

### 不兼容点 3：旧版 Raylib 交互要迁移到 dxgfx / Win32

旧版用了 Raylib：

```c
InitWindow
BeginDrawing
ClearBackground
BeginMode2D
GetScreenToWorld2D
GetMouseWheelMove
IsKeyPressed
IsKeyReleased
```

新版没有 Raylib。  
我建议继续扩展 `dxgfx.cpp`，不引入 Raylib 依赖。

请确认是否允许修改 `dxgfx.cpp / dxgfx.h` 来补完整窗口、输入、帧绘制接口。

### 不兼容点 4：指令名表来源

旧版有大量：

```c
start_ins_name
raylib_list
stdio_list
stdlib_list
...
```

新版目前没有统一 mod name registry，mod 只有 hash 文件名。

完整版编辑器需要显示可插入指令名。方案有两个：

#### 方案 A：内置一个新版 mod registry block

用 block/payload 存：

```text
name -> token[32]
```

编辑器从 registry block 读指令列表。

优点：符合新版数据化架构。  
缺点：需要做 registry 相关 mod。

#### 方案 B：editor mod 内置基础指令表

把常用 mod 名和 token 编译进 editor 相关 mod。

优点：快。  
缺点：扩展性差，比较像旧版硬编码。

我建议方案 A。

请确认选 A 还是 B。

---

## 我建议的完整版实现路线

如果你确认上面设计，我会直接开始做完整版，但分成这些内部阶段提交，不再停在“最小版”：

### 阶段 1：新版 editor runtime

- 完整窗口帧循环；
- 独立 editor state；
- 输入事件 pressed/released/text；
- camera / view / mouse world 坐标。

### 阶段 2：block view 渲染

- 多 view；
- 绘制 block 指令列表；
- 显示 token 名称；
- 显示 payload 摘要；
- 鼠标选择 point；
- 线条连接 block/function。

### 阶段 3：编辑操作

- 自动插入指令；
- 插入 block/function；
- 插入 data；
- 删除；
- 复制；
- 粘贴；
- 回车/Tab/Alt 等快捷键；
- 修改后写回服务器。

### 阶段 4：补全和命令输入

- 输入字符串；
- 指令名匹配；
- 补全；
- 数字自动 data；
- remove underscores。

### 阶段 5：接近旧版交互

- 多 view 拖动；
- 鼠标中键移动 camera；
- 右键拖 view；
- 滚轮缩放；
- window close 退出；
- 可持续 reexec。

---

## 请确认

你只需要回复下面几项即可：

1. 是否允许继续新增上述 editor/gfx/input/block 系列 mod？
2. 是否允许修改 `dxgfx.cpp / dxgfx.h`？
3. 函数/块是否按新版 `logical block key[32]` 表示，而不是旧版内存地址？
4. 指令名 registry 选：
   - A：registry block 数据化；
   - B：editor mod 内置表。

我拿到确认后就直接做完整版。

## user

1，允许但指令token语义尽量宽
2. 允许修改 `dxgfx.cpp / dxgfx.h`
3，新版的
4，从网络遍历，从hash(‘#TAG’)开始，如果每个child的内容以#开头，就代表标签，没有就是指令token