# Singularity 项目接续工作 - 详细上下文

## 项目位置
C:\Users\12159\Desktop\Singularity

## 项目概述
Singularity是一个基于VM的自编辑系统。vm.exe连接远程服务器(118.25.42.70:9000)，下载block数据，加载DLL mod执行。编辑器(editor_frame.c)允许用户查看和修改block中的指令序列。

## 文件结构
- vm.c - 主程序（不可修改），连接服务器，下载block，加载DLL，for(;;) imp()循环
- vmexec.c - 指令分发：find(DLL)或resolve(block)
- vmstore.c - 网络+缓存：upload/download，SHA256，user override，8条目LRU缓存 + 文件系统缓存
- vmstate.c - 调用栈帧，当前指令指针ptr
- vmvar.c - 作用域变量
- cont.c - 推进到下一条指令，block结束时cvm_ret()恢复帧
- dxgfx.cpp - Direct2D渲染，1280x720，Consolas字体
- dxgfx.h - 渲染API头文件
- mods_src/ - 所有mod源码（55个.c文件）
- mods/ - 编译后的DLL，按SHA256哈希命名（467个.dll文件）

## 编译命令
```
# vm.exe（主程序，不可修改）
gcc vm.c -o vm.exe -Wl,--out-implib,libvm.a -lws2_32

# vmstore.dll（缓存+网络）
gcc -shared vmstore.c -o vmstore.dll libvm.a -Wl,--out-implib,libvmstore.a -lws2_32 -ladvapi32

# editor_frame.dll（编辑器）
gcc -shared mods_src/editor_frame.c -o mods/editor_frame.dll libcont.a libvmstate.a libvmstore.a libvm.a libdxgfx.a -lws2_32 -ladvapi32 -luser32

# 替换DLL到hash命名（编译后需要）
# 复制editor_frame.dll到3个hash命名的文件
```
**注意**：gcc需要PATH包含C:\mingw64\bin，subprocess调用时需设置env['PATH']

## 已完成的修改

### 1. vmstore.c - 8条目LRU缓存（已完成，正常工作）
- 原: 单条目缓存 (cache_key, cache_raw[1MB])
- 新: 8条目LRU缓存 (slots[8], 每个1MB)
- 命中: 线性扫描8个slot, 更新primary_idx (无memcpy)
- 未命中: 淘汰最久未用的slot
- 文件系统缓存: file_get() 和 cvm_children() 都有磁盘缓存 (cache/ 目录)
- 效果: 启动从2-3分钟降到秒开

### 2. editor_frame.c - 编辑器完整实现（约604行）

**a) PE导出签名名称解析系统（核心功能，已验证）**
- `pe_read_exports()` - 读取DLL的PE导出函数名
- `nc_scan_mods()` - 启动时扫描mods/目录，两层匹配策略：
  - Tier 1: 唯一签名匹配（如 `editor_state_init,run` → `editor_frame`）
  - Tier 2: 签名+文件大小匹配（如 `run+38298` → `editor_init`）
- 结果: 466个DLL全部解析成功，block指令显示可读名称
- name_cache.bin 持久化缓存，避免重复扫描

**b) 右键上下文菜单**
- 右键点击有payload>=32的指令
- 弹出 "Open block in new view" 菜单
- 点击创建新视图

**c) HUD位置修复**
- dxgfx_set_camera(640.0f,360.0f,1.0f) 用于屏幕空间HUD

## 服务器数据结构（用singularity_net.py验证）
```
Root block: 5 children, first=bootstrap(46e3a507...)
Bootstrap: 5 children, first=editor_block(bed0ab62...)
Editor block: [b31b63d8] [c549153c] [9cc4dbcf] [000...]
#TAG: 64 children (registry entries)
```

## 工具和Skills
路径: C:\Users\12159\Desktop\ae_agent\skills\

| 模块 | 用途 |
|------|------|
| expert.py | 专家模型咨询 (GPT-5.5等) - `from skills.expert import ask` |
| win_gui.py | Windows GUI自动化 - `from skills.win_gui import launch, find_window, screenshot` |
| net_proto.py | TCP协议客户端 - `from skills.net_proto import Client` |
| build.py | C/C++编译部署 - `from skills.build import CCompiler, deploy_dll` |
| pe_utils.py | PE分析 - `from skills.pe_utils import pe_exports, find_strings` |
| file_ops.py | 文件工具 - `from skills.file_ops import read_log, read_binary_struct` |

## API配置
- GPT-5.5: key=sk-ecf7d70c71c8df2063f0045ef2efbeadf962f243f45db132e5eb5796de123b6f, url=https://uuapi.net/v1
- mimo-v2.5: key=sk-caj5lzipzvsfqiirld3txhns9e8jmkvp3lj8pagse21bkmk7

## 约束
- vm.c 不能修改
- 界面体验尽量接近旧版 transition/main.c 的 to_dest_dev_base
- 用户偏好: 性能改进不动基本逻辑，改动前需确认
- 每次修改必须求助GPT-5.5（hight推理强度），除非非常确定
- 工具调用绝不重复实现，必须添加或使用skills

## 下一步优先级
1. **验证右键菜单** - 右键点击有payload>=32的指令，检查"Open block in new view"菜单
2. 旧版其他特性（缩进、颜色、连接线等）- 待用户确认后实现
3. 优化nc_scan_mods性能（当前约1秒，可考虑增量扫描）

## 当前状态
- vm.exe秒开正常（8条目LRU缓存 + 文件系统缓存）
- **编辑器指令名称全部正确显示**（466个DLL名称解析成功）
- 右键菜单已实现（Open block in new view）
- name_cache.bin 缓存466条记录，启动秒开
