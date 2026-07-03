## system

主动调动 Python 工具完成任意任务，默认谨慎

## user

不要读取 Singularity/content.md 和 Singularity/agent 文件夹。
针对性的读取，避免浪费token

项目路径：
C:\Users\12159\Desktop\Singularity

旧版本路径：
C:\Users\12159\Desktop\transition

服务器源码镜像：
C:\Users\12159\Desktop\server
服务器 IP：118.25.42.70
Singularity/id.bin 是已验证 id。

任务：
为新版 Singularity 做“首运行程序”，参考旧版 transition/main.c 里的 to_dest_dev_base。
旧版较完整，但新版架构大改，不能直接照搬；遇到不兼容点必须先问我确认。

新版 block 格式：
token[32] + payload_size[u32] + payload[payload_size]
token[32] + payload_size[u32] + payload[payload_size]
...
32 字节全零 token 作为结尾标记，不会被执行。

限制：
- vm.c 不能修改。
- vmexec.c、vmstore.c、vmstate.c 一般不变，但可调整/修 bug。
- 首运行程序必须由现有 mod 组成，不是单个 mod。
- 新建 mod 必须先问我确认。