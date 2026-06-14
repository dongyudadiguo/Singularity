# CVM — Content-Addressable Virtual Machine

极简内容可寻址虚拟机。Win11 + MinGW-w64，纯二进制 TCP 协议，指令从 `mods/*.dll` 动态加载。

## 架构

```
┌─────────────┐   binary TCP    ┌─────────────┐
│   cvm.exe    │ ◄──────────────► │  server.go   │
│  (MinGW-w64) │  op+len+body    │   :9000      │
└──────┬───────┘                 └─────────────┘
       │ LoadLibrary
┌──────┴──────────────────────────────────┐
│  mods/*.dll (按文件名排序加载)            │
│  00_core  · 01_graph  · 02_block        │
│  03_runtime · 04_data · 05_env          │
└─────────────────────────────────────────┘
```

## 快速开始

### 依赖
- **Windows 11** + **MinGW-w64** (gcc)
- **PowerShell** (构建脚本)
- **Python 3** (生成 & 部署 zero)
- **Go** (编译服务器，可选)

### 1. 编译

```powershell
.\build.ps1
```

产出：
- `cvm.exe` — VM 本体
- `mods/00_core.dll` — 栈、字节、数、hash、token
- `mods/01_graph.dll` — 图操作（上传、边、投票等）
- `mods/02_block.dll` — 块编辑（插入、删除、修改）
- `mods/03_runtime.dll` — 执行、跳转、帧、变量
- `mods/04_data.dll` — 列表、键常量
- `mods/05_env.dll` — UI、绘制、输入、时间、文件系统

### 2. 部署首启动程序

```bash
python make_zero.py
```

连接到 `118.25.42.70:9000`，上传 zero.bin、建 edge、注册临时用户并投票。

### 3. 运行

```bash
.\cvm.exe
```

弹出窗口，显示块浏览器。

## 服务器

```bash
go build -o oneserver main.go
sudo systemctl restart oneserver   # 部署在 118.25.42.70:9000
```

状态持久化到 `cvm.gob`。

## 协议

纯二进制 TCP 帧，无 HTTP / JSON：

```
请求:  op:u8 + len:u32be + body
响应:  status:u8 + len:u32be + body
```

| 操作码 | 名称 | 输入 | 输出 |
|--------|------|------|------|
| 1 | REGISTER | turnstile token | identity[32] |
| 2 | UPLOAD | file bytes | hash[32] |
| 3 | FILE | hash[32] | file bytes |
| 4 | EDGE | parent[32]+child[32] | — |
| 5 | CHILDREN | parent[32] | count(u32be) + repeated child[32]+score(i64be) |
| 6 | VOTE | user[32]+parent[32]+child[32] | — |
| 7 | USET | user[32]+key[32]+value[32] | — |
| 8 | UGET | user[32]+key[32] | value[32] |

## VM 核心设计

- **无预设指令**：所有指令从 DLL 动态注册，后注册可覆盖前注册
- **Op = void**：指令无返回值，普通指令显式调用 `h->adv()` 推进
- **块格式**：`[32 byte token] [4 byte span (u32le)] [payload...]`，结束符 32 零字节
- **栈**：256 深度，存任意字节
- **变量**：512 槽位，key 为 32 字节 token
- **本地覆盖**：`OV:SET` / `OV:TOUCH` 可覆盖任意 key 对应的文件
- **单文件**：cvm.c 无独立头文件，所有类型内联

## 指令集

### 00_core — 核心操作
| 前缀 | 指令 |
|------|------|
| `ST:` | PUSH, POP, DUP, SWAP |
| `BY:` | LEN, CAT, SLICE, CMP, TAKE32, HEX, UNHEX |
| `U32:` | CONST, ADD, SUB, MUL, DIV, MOD, EQ, LT, GT, AND, OR, XOR, SHL, SHR, NOT, FROMBE, TOBE, DEC |
| `HASH:` | SHA256 |
| `TOK:` | ZERO, MAKE, TEXT, ISZERO, EQ |

### 01_graph — 图操作
| 前缀 | 指令 |
|------|------|
| `G:` | REGISTER, UPLOAD, FILE, CHILDS, CHILD0, EDGE, VOTE, UGET, USET |
| `CH:` | COUNT, FIRST, HASH, SCORE, ROW, HASHES |

### 02_block — 块编辑
| 前缀 | 指令 |
|------|------|
| `BLK:` | COUNT, HASH, DATA, ITEM, END, INS, DEL, SET |

### 03_runtime — 运行时
| 前缀 | 指令 |
|------|------|
| `RUN` `ENTER` `ADV` | 执行控制 |
| `OV:` | SET, TOUCH |
| `FLOW:` | JMP, JREL, JZ, JNZ, NEXT, END |
| `CUR:` | FILE, KEY, OFF, SETOFF |
| `VAR:` | SET, GET, HAS, DEL, CLEAR |

### 04_data — 数据结构
| 前缀 | 指令 |
|------|------|
| `LST:` | NEW, COUNT, GET, PUSH, DEL, JOIN |
| `KEY:` | ESC, ENTER, BACK, DEL, TAB, SPACE, LEFT, RIGHT, UP, DOWN, HOME, END, PGUP, PGDN, CODE, ASCII, MODS |

### 05_env — 环境
| 前缀 | 指令 |
|------|------|
| `UI:` | OPEN, POLL, SIZE, CLOSE |
| `DRAW:` | CLEAR, TEXT |
| `IN:` | KEY |
| `CLIP:` | GET, SET |
| `TIME:` | MS, SLEEP |
| `FS:` | READ, WRITE, EXISTS, CWD |

## 文件清单

```
├── cvm.c              VM 单文件源码
├── server.go          服务器源码
├── build.ps1          PS 编译脚本
├── make_zero.py       生成 & 部署 zero
├── mods_src/
│   ├── 00_core.c      栈/字节/数/hash/token
│   ├── 01_graph.c     图操作
│   ├── 02_block.c     块编辑
│   ├── 03_runtime.c   执行/跳转/帧/变量
│   ├── 04_data.c      列表/键常量
│   └── 05_env.c       UI/绘制/输入/时间/文件
└── mods/              编译产出的 DLL
```

## 设计原则

- **纯二进制最低传输**：无 HTTP/JSON/hex 路径
- **从下到上设计**：指令为最小原子操作，禁止应用层复合指令
- **跨平台语义**：token 无平台前缀（`UI:OPEN` 而非 `WIN:WINOPEN`）
- **崩溃不管**：无错误处理，依赖系统轮子
- **单文件优先**：VM 本体单文件，mods 各自独立