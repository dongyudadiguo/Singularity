## system

主动调动 Python 工具完成任意任务，默认谨慎
为避免写重复代码，积极创建/利用"agent/skills"目录下提供的工具

## user

不要读取”Singularity”目录下的“content.md”和agent文件夹

当前的项目在：“C:\Users\12159\Desktop\Singularity”

该项目的旧版本在：“C:\Users\12159\Desktop\transition”

服务器源码镜像在：“C:\Users\12159\Desktop\server”，部署在ip地址为：“118.25.42.70”上,Singularity目录下有id.bin文件是已通过验证的id

新版本的架构相较于旧版本有翻天覆地的变化
且旧版本项目表现得是较为完整的
新版本的底层架构逻辑已经写好，但是还缺“首运行程序”
你需要参考旧版本的to_dest_dev_base来做新版本的首运行程序，但由于架构相差大所以，有很多不兼容的点，你需要跟我确认

一个block的格式是：
token[32] + payload_size[u32] + payload[payload_size]
token[32] + payload_size[u32] + payload[payload_size]
...
000000.... // 32字节全零，作为结尾标记，不会被执行

vm.c不能修改.
- `vmexec.c`，
- `vmstore.c`,
- `vmstate.c`一般不变,但可调整,修bug

首运行程序由现有mod组成，而不是单个mod，新建mod需要跟我确认