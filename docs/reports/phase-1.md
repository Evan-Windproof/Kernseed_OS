# Phase 1 阶段报告

日期：2026-11-04（Asia/Taipei）。状态：本地实现与阶段测试完成，等待发起人审查。
这是第一阶段报告，**不是整个 Kernseed OS 项目完成报告**。没有执行远程写入、push、release 或归档。

## 已实现

- Limine v8.7.0 固定提交与 SHA-256 核对；可重建 BIOS/UEFI 布局的 ISO，实际验收为 BIOS。
- 高半区静态 ELF64、freestanding C、无 libc/动态链接、分离的 ELF 段权限。
- 独立 x86-64 汇编入口，CLI/CLD、自有 64 KiB 栈、SysV C 调用对齐。
- 自有 `memcpy/memmove/memset/memcmp`，BSS 与内核内 runtime 自检。
- 16550 COM1 轮询日志及 loopback 检查；等待有上限。
- 实际 framebuffer 文本输出，原创 5×7 字体、2× 放大、RGB 格式验证、换行/回绕清屏。
- 读取真实 CR0/CR3/CR4/EFER/RFLAGS；可观察 `boot_state`；明确 panic 与稳定 HLT 路径。
- 构建/启动/调试/测试脚本、模块依赖、阶段路线与教学文档。
- 完整原样 PolyForm Noncommercial 1.0.0；保留 Limine 许可证和镜像内通知。

## 实际环境和依赖

实际机器是 x86-64 Debian 13 云容器，**不是 WSL 2 Ubuntu**。
GCC 14.2.0、GNU ld 2.44、Make 4.4.1 已预装。
补充 QEMU 10.0.13、GDB 16.3、xorriso 1.5.6、SeaBIOS 及其运行依赖：
使用 Debian 官方 trixie / trixie-security 签名索引，APT 核对软件包后解包到
`/workspace/kernseed-tools/root`，未关闭 TLS、签名或 checksum 检查。
当前 `config.local.mk` 指向工作区工具 wrapper；不把机器绝对路径提交为项目依赖。

WSL/Ubuntu 提供 `sudo sh scripts/setup-host.sh` 安装方式，但本次没有在 WSL 中执行，
也没有声称已验证 Ubuntu root 安装脚本。实际构建、模拟与 GDB 测试均在上述云机器执行。

## 测试与结果

最终直接执行 `make test`，退出状态 0；原始结果位于
`build/test-results/20261103T213550Z/`（目录名为 UTC）。
持久审查副本见 [evidence/phase-1](evidence/phase-1/)。

| 检查 | 实际验证 | 结果 |
|---|---|---|
| 干净构建 | 空 build 目录，GCC/ld/xorriso/Limine 安装启动扇区 | 通过 |
| 空缓存依赖 | 重新从官方仓库 fetch 固定 SHA，逐文件 hash 验证，构建安装工具 | 通过 |
| ELF | ELF64 EXEC、x86-64、无未解析符号、无 INTERP/DYNAMIC、无 RWX 段 | 通过 |
| C runtime 宿主测试 | 12,464 种 memmove 重叠/边界组合，memcpy/memset 哨兵及比较自检 | 通过 |
| framebuffer 宿主测试 | RGB 通道顺序、实际字形像素、padding/边界哨兵、回绕、11 类拒绝输入 | 通过 |
| 三次冷启动 | 各自新建 TCG/SeaBIOS 单核进程，64/128/256 MiB RAM | 全部通过 |
| 屏幕输出 | QMP 截图的 800×600 像素、K 字形和最后状态行；实际查看截图 | 通过 |
| idle 稳定性 | 每个正常启动后额外等待 2 秒，检查 QMP running 和 CPU HLT=1 | 通过；不是长时间耐久测试 |
| GDB C 入口 | 第一条指令断点，RSP 对齐/栈边界、DF/IF、BSS probe | 通过 |
| GDB 完成状态 | runtime 返回值、真实 CR3、PG/PAE/LME/LMA、framebuffer、stage=2 | 通过 |
| GDB idle | 硬件断在真实 HLT 指令，检查相邻跳转，detach 后继续停机 | 通过 |
| 手动调试脚本 | 实际执行 `make debug` 与 `scripts/debug.gdb`，TCP 本机断点后 detach，继续到 BOOT_READY | 通过；测试后显式停止所启动进程 |
| 无 framebuffer | `-vga none`，真实串口 `PANIC: No framebuffer response`，无 BOOT_READY | 通过预期失败路径 |
| 重建一致性 | 同路径/工具链两次强制重建并比较 ELF 与 ISO SHA-256 | 通过 |

GDB 本次观测示例：CR0=`0x80010011`，CR4=`0x20`，EFER=`0xd00`，
CR3=`0x7f55000`。这些是该次运行的读数，其他启动不保证 CR3 相同。
截图见 [screen.png](evidence/phase-1/screen.png)，原始寄存器和串口日志均保留。

镜像：`build/kernseed.iso`；调试 ELF：`build/kernseed.elf`。
最终哈希见 [artifact-sha256.json](evidence/phase-1/artifact-sha256.json)。
重建一致性只覆盖相同绝对路径、工具链和依赖，不保证跨主机/工具版本哈希相同。

## 遇到的问题与修复

| 问题 | 诊断与修复 |
|---|---|
| 无 root、缺少模拟与调试工具 | 使用 APT 验证包并工作区解包；wrapper 加载本地库，配置本地工具路径 |
| QEMU 找不到 `bios-256k.bin` | 解包安装的固件搜索路径不同；wrapper 增加 SeaBIOS 与 QEMU 数据目录 |
| 沙箱禁止 QMP UNIX socket | 保留 Operation not permitted 日志；通过获准的沙箱外执行完成测试，无禁用断言 |
| GDB 地址寄存器与 EFLAGS 类型不兼容 | RSP 显式转换；EFLAGS 专用 flags 类型改读 `info registers` 实际十六进制值，继续验证 DF/IF |
| C 行断点可能落在序言之后 | 使用 `hbreak *kernel_main` 在第一条指令检查 ABI 栈，避免错判 |
| ISO 两次重建哈希不同 | 字节差异定位到随机 GPT GUID 和其 CRC；固定 GUID、日期，重建比较通过 |
| PolyForm 正式网页返回 403，猜测的 txt 路径不存在 | 从 PolyForm 官方 Git 仓库取得准确 `.md` 文件，原样复制并验证 SHA-256 一致 |

早期失败日志副本在 [failures](evidence/phase-1/failures/)。
未把 timeout 终止的初次手动启动当作测试通过；最终测试由 QMP 正常退出，检查子进程退出码。

## 尚未实现与未执行

- Phase 2–7：自有 GDT/IDT、异常/硬件 IRQ、PIT、页帧分配/页表修改、真实任务切换、
  键盘输入、Shell、系统调用、全系统集成回归。
- 未进行 UEFI/OVMF、KVM、SMP、实机、WSL 2 或长时间耐久验收。
- 当前无用户态，不提供用户进程隔离；全部执行为 CPL0，沿用引导器页表。
- 无输入命令，不能使用 help/meminfo/tasks/uptime 等未来 Shell 接口。
- ELF 权限检查不代替未来的 MMU 权限测试；字节复制测试不代替页分配测试。

## 人工事项与下一阶段

本阶段无未解决的环境或内核启动阻塞。发起人可审阅源码、报告和镜像，
在目标 WSL 2 环境复核工具安装与显示后端。任何 GitHub 推送、发布或归档仍需明确授权。
下一阶段按依赖顺序实现自有 GDT/IDT、异常现场与 PIC/PIT，再做受控异常和真实定时器 IRQ 验收。
