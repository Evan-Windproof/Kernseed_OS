# Kernseed OS

**一个面向操作系统入门教学的独立 x86-64 宏内核实验项目。**

Kernseed OS 使用 **C11 + 少量 x86-64 汇编**编写内核，由 **Limine** 引导，借助 **QEMU** 启动、**GDB** 调试。项目由 Codex 主导实现，发起人进行构建、运行与审查。它不是 Linux 发行版，也不依赖 Linux 用户空间；与 Orfreon、Evavind 等其他项目无关。

> **当前版本：Phase 2（CPU 异常、中断与 PIT 定时器）**
> 已能从 ISO 启动，加载自有 GDT/TSS/IDT，处理 CPU 异常并通过 IRQ0 维护真实 tick；**尚无键盘输入、Shell、调度器、内存分配器或文件系统**。屏幕停留在 `BOOT READY - ENTERING IDLE LOOP` 属于预期结果，而非死机。
>
> 仅供学习与研究，不适用于生产环境；项目计划在教学目标完成后停止长期维护。

## 目录

1. [五分钟快速开始](#五分钟快速开始)
2. [首次安装与 WSL-2-准备](#首次安装与-wsl-2-准备)
3. [编译内核与生成 ISO](#编译内核与生成-iso)
4. [启动与退出 QEMU](#启动与退出-qemu)
5. [使用 GDB 调试](#使用-gdb-调试)
6. [当前功能与启动画面](#当前功能与启动画面)
7. [测试、清理与重建](#测试清理与重建)
8. [常见问题排查](#常见问题排查)
9. [项目结构与学习路线](#项目结构与学习路线)
10. [许可证与免责声明](#许可证与免责声明)

## 五分钟快速开始

以下命令在 **WSL 2 Ubuntu 的终端**运行。要求已将仓库克隆到本地且能访问软件包源和 GitHub。

**你也可以去Releases下载已经生成好的.iso文件，这样就可以跳过一切生成.iso文件的步骤。**

```bash
# 获取源码（若已下载 ZIP 并解压，可跳过此步）
git clone https://github.com/Evan-Windproof/Kernseed_OS.git
cd Kernseed_OS

# 安装依赖
sudo sh scripts/setup-host.sh

# 下载并验证 Limine，编译内核和 ISO
make deps
make -j2

# 启动 QEMU 图形窗口
make run
```

成功时，QEMU 内会显示 `KERNSEED OS - PHASE 2`、CPU 与 framebuffer 信息，以及 `BOOT READY - ENTERING IDLE LOOP`。**此阶段不能输入命令**。关闭 QEMU 窗口即可退出；若终端仍运行，按 `Ctrl+C`。

## 首次安装与 WSL 2 准备

### 推荐环境

- Windows 11 + WSL 2 Ubuntu（推荐使用项目目录 `~/Projects/Kernseed_OS`）；或原生 x86-64 Debian/Ubuntu。
- 可用的 GCC、GNU ld、Make、Git、Python 3、xorriso、QEMU 和 GDB。
- 首次下载 Limine 时需要联网。
- QEMU 默认使用 **TCG 软件模拟**，不要求 WSL 2 中存在 `/dev/kvm`。

在 WSL 2 Ubuntu 中运行：

```bash
sudo sh scripts/setup-host.sh
```

该脚本会调用 APT 安装 `build-essential`、`binutils`、`git`、`ca-certificates`、`python3`、`qemu-system-x86`、`gdb` 与 `xorriso`。如果脚本无法运行，也可以先阅读 `scripts/setup-host.sh`，再按需手动安装软件包。

**若从 ZIP 解压源码：** 把内容放到 Linux 文件系统（如 `~/Projects/Kernseed_OS`），并进入含有 `Makefile` 的项目根目录。Windows 压缩/解压有时会丢失脚本的可执行权限；遇到 `Permission denied` 时请参见[常见问题](#常见问题排查)。

检查工具是否可用：

```bash
gcc --version
ld --version
qemu-system-x86_64 --version
gdb --version
```

## 编译内核与生成 ISO

在仓库根目录执行：

```bash
make deps       # 固定版本 Limine：下载、SHA-256 校验、构建引导工具
make -j2       # 默认目标为 iso；自动编译内核并打包 ISO
```

也可以直接：

```bash
make iso
```

成功后应看到：

| 文件 | 说明 |
|---|---|
| `build/kernseed.elf` | 带调试符号的 x86-64 内核 ELF，供 GDB 使用 |
| `build/kernseed.iso` | 可以交给 QEMU 的可启动光盘镜像 |

验证文件存在：

```bash
ls -lh build/kernseed.elf build/kernseed.iso
file build/kernseed.elf build/kernseed.iso
```

`make` 不等于从 ZIP “挂载”系统：它先将源码编译链接为 ELF，再将 ELF 与 Limine 引导文件打包为 ISO。**不要直接把源码 ZIP 改名为 `.iso`**。

## 启动与退出 QEMU

### 方式 A：图形窗口（推荐初学者）

```bash
make run
```

项目的 `scripts/run.sh` 默认使用 QEMU GTK 图形后端，将串口输出保留在运行命令的终端。在 WSL 2 下需要 WSLg 正常工作。若未安装图形后端，可在 Ubuntu 中安装 `qemu-system-gui`，或直接使用下面的无窗口模式。

### 方式 B：无图形窗口，仅观察串口

```bash
make headless
```

当前内核同时向 COM1 输出启动日志，无窗口模式也能够验证启动。按 **`Ctrl+C`** 结束。

### 方式 C：直接加载生成的 ISO

```bash
qemu-system-x86_64 \
  -machine pc,accel=tcg -cpu qemu64 \
  -m 128M -smp 1 -vga std \
  -cdrom build/kernseed.iso -boot d \
  -nic none -serial stdio -monitor none \
  -no-reboot -no-shutdown
```

这是项目运行脚本采用的基本配置，便于学习 QEMU 参数。不要在普通运行时添加 `-S`：它会让 CPU 启动后暂停，直到 GDB 发出 `continue`。

退出方式：关闭 QEMU 图形窗口，或在启动它的终端按 `Ctrl+C`。窗口顶部提示 `Ctrl+Alt+G` 是**释放鼠标/键盘抓取**，不是关机命令。

## 使用 GDB 调试

请打开 **两个 WSL 2 终端**，都切换到 Kernseed OS 项目根目录。

终端一：启动暂停的 QEMU，监听本机调试端口：

```bash
make debug
```

终端二：让 GDB 加载带符号的内核，连接 QEMU，并在 `kernel_main` 第一条指令处设置硬件断点：

```bash
gdb -x scripts/debug.gdb
```

项目脚本已包含 `file build/kernseed.elf` 和 `target remote 127.0.0.1:1234`。**不要再次手动执行 `set architecture i8086`**；Kernseed OS 当前使用 x86-64 长模式，与早前 16 位引导扇区实验不同。

常用 GDB 指令：

| 指令 | 作用 |
|---|---|
| `info registers` | 查看 CPU 寄存器 |
| `info registers rip rsp cr0 cr3 cr4` | 查看指令、栈与分页相关寄存器 |
| `x/8i $rip` | 反汇编当前位置附近 8 条指令 |
| `x/8gx $rsp` | 查看栈顶 8 个 8 字节数据 |
| `si` | 单条机器指令执行 |
| `ni` | 单步执行且尽量跳过函数调用 |
| `hbreak kernel_idle` | 在空闲函数设置硬件断点 |
| `continue` 或 `c` | 继续运行虚拟 CPU |
| `Ctrl+C` | 中断正在运行的被调试程序 |
| `quit` | 退出 GDB |

**注意：** 脚本会连接后自动继续执行，直到命中入口断点。若停在入口断点，输入 `continue` 便可让内核继续启动。调试端口只应监听 `127.0.0.1`，不要对公网开放。

## 当前功能与启动画面

### Phase 1 已实现

- Limine 引导和独立 x86-64 C/汇编内核入口。
- 自有启动栈（64 KiB）、基本 C 内存例程、BSS 与 C 运行环境自检。
- COM1 轮询串口输出，以及 framebuffer 字符绘制。
- 读取部分真实 CPU 状态（如 `CR3`）并显示启动信息。
- HLT 空闲循环及 GDB 调试基础设施。

### Phase 2 新增

- 自有 64 位 GDT、TSS、256 项 IDT；Double Fault 使用独立 16 KiB IST 栈。
- 汇编入口保存全部通用寄存器，规范化错误码，遵守 C 栈对齐并通过 `iretq` 返回。
- CPU 异常输出向量、错误码、RIP、RSP；页故障额外输出 CR2。不可恢复异常受控停机。
- 8259 PIC 重映射，只放行 PIT IRQ0；11932 分频（约 100 Hz），真实递增 tick 和 EOI。
- CPL0 软件 `INT 0x80` 验证寄存器/DF 恢复，仅供内部自检，**不是系统调用接口**。
- 五类真实故障使用独立测试镜像；正式启动不触发危险异常。

启动画面示意（实际寄存器地址会因运行而变化）：

```text
KERNSEED OS - PHASE 2
BOOT: LIMINE BASE REVISION 3
BSS: ZERO CHECKED
C RUNTIME: SELFTEST PASSED
CPU: LONG MODE  PAGING  INITIAL IF=0
CR3: 0X...
FRAMEBUFFER: 800X600X32
NO SCHEDULER, KEYBOARD OR SHELL YET
GDT TSS IDT: LOADED, DF IST READY
SOFTWARE INT: REGISTER/IRET SELFTEST PASSED
PIT: IRQ0 ONLY, DIVISOR=11932 (ABOUT 100 HZ)
BOOT READY - ENTERING IDLE LOOP
```

### 目前不能做什么？

- **不能输入 Shell 命令**，因为还没有键盘驱动与命令解释器。
- **没有任务调度**、内核页框分配器、文件系统、用户态进程或应用程序执行环境。
- 当前页表仍由引导器准备，尚无自有虚拟内存管理器；没有用户态隔离、APIC/SMP 或异常恢复机制。
- 屏幕启动文字不是实时 tick 仪表。可在 GDB 中读取 `irq_state.ticks`；CPU 暂停时 tick 不会增长。

因此，**Kernseed OS Phase 2 是具备基础中断机制的教学内核，不是完整的日用操作系统。**

## 测试、清理与重建

```bash
make test           # Phase 1 回归 + Phase 2 描述符/IRQ0/五类隔离异常测试
make test-host      # 仅运行宿主上的 C 代码测试
make check          # 检查 ELF 属性
make reproducible   # 同工具链和目录下两次构建比较哈希
make clean          # 删除 build/ 构建产物
make -j2            # 再次构建
```

Phase 1 回归日志与截图保存在 `build/test-results/`，Phase 2 日志保存在 `build/phase2-results/`。
异常镜像位于 `build/fault-1/` 到 `build/fault-5/`，不会替换正常 ISO。
只运行 Phase 2 测试：先 `make iso`，再 `make test-phase2`。如失败，应阅读具体错误日志，而不是把编译成功当作全部测试通过。`make clean` 不删除 `.cache/` 下载依赖。

当前项目曾在 Debian 13 云环境进行 Phase 1 测试；发起人随后已在 WSL 2 Ubuntu **本地成功编译 ISO，并通过 QEMU 图形窗口看到与云端一致的 Phase 1 启动画面**。这证明该配置下的 Phase 1 基本启动流程可复现，但不代表所有自动测试均已在 WSL 2 上执行。
Phase 2 本次在 Debian 13 云环境的 QEMU TCG/SeaBIOS 单核配置实测；尚未在 WSL 2、UEFI、实机或 SMP 上验收。

## 常见问题排查

| 现象 | 可能原因与处理 |
|---|---|
| `./scripts/fetch-limine.sh: Permission denied` / `Error 126` | ZIP 解压后执行权限丢失：运行 `chmod +x scripts/fetch-limine.sh scripts/run.sh scripts/check-reproducible.sh`；按错误提示仅修复需要执行的脚本。 |
| `make: No rule to make target ...` | 未处于包含 `Makefile` 的项目根目录；先运行 `pwd`、`ls`。 |
| `qemu-system-x86_64: command not found` | 安装 `qemu-system-x86`。 |
| QEMU GTK / display 初始化失败 | 可能缺少 `qemu-system-gui` 或 WSLg 显示环境；试 `make headless`。 |
| 窗口显示 `[Paused]`、黑屏 | 使用了 `make debug` 或 `-S`，等待 GDB 连接并执行 `continue`；普通运行使用 `make run`。 |
| `Connection refused`（GDB） | 未运行 `make debug`，或调试端口不是 `127.0.0.1:1234`。 |
| GDB 报 `i8086` 与 `x86-64` 不匹配 | 不要沿用 16 位引导扇区实验的 `set architecture i8086`；使用仓库的 `scripts/debug.gdb`。 |
| 屏幕已显示 `BOOT READY` 却不能输入 | 正常：当前尚未实现键盘与 Shell。 |
| Limine 下载失败 | 检查 GitHub 网络连接、`git` 与 CA 证书；不要关闭哈希验证。 |

若 ZIP 解压造成其他 `.sh` 文件权限问题，可执行 `ls -l scripts/` 查看，再对确实需要直接执行的脚本逐个 `chmod +x`；`sudo sh scripts/setup-host.sh` 本身不需要执行位。

## 项目结构与学习路线

```text
Kernseed_OS/
├── boot/                # Limine 配置、链接脚本
├── kernel/
│   ├── arch/x86_64/     # 汇编入口与底层 CPU 操作
│   ├── core/            # 内核启动、基础 C 运行函数
│   ├── drivers/         # framebuffer、串口
│   └── interrupts/      # IDT、异常诊断、PIC/PIT
├── include/kernseed/    # 内核头文件
├── third_party/limine/  # Limine 协议头和第三方许可
├── scripts/             # 下载依赖、运行、调试与构建辅助
├── tests/               # 宿主测试和 QEMU 自动测试
├── docs/                # 架构、启动、调试与阶段报告
├── Makefile
├── README.md
├── LICENSE
└── THIRD_PARTY_LICENSES.md
```

教学文档入口：

- [架构与路线图](docs/architecture.md)
- [启动流程](docs/boot-process.md)
- [QEMU 操作](docs/qemu-guide.md)
- [GDB 调试](docs/gdb-guide.md)
- [内存管理的现状和计划](docs/memory-management.md)
- [中断机制的现状和计划](docs/interrupts.md)
- [调度器设计计划](docs/scheduler.md)
- [Codex 开发实验记录](docs/codex-experiment.md)
- [Phase 1 开发报告](docs/reports/phase-1.md)
- [Phase 2 开发报告](docs/reports/phase-2.md)

预定学习阶段：Phase 1 引导 → Phase 2 中断与定时器 → Phase 3 内存管理 → Phase 4 内核任务调度 → Phase 5 键盘与 Shell → Phase 6 系统调用接口 → Phase 7 集成测试和教学发布。**Phase 1 与 Phase 2 已实现；Phase 3–7 仍是后续计划。**

## 许可证与免责声明

Kernseed OS 的项目自有源码采用 **[PolyForm Noncommercial License 1.0.0](LICENSE)**，详见 [许可证官方说明](https://polyformproject.org/licenses/noncommercial/1.0.0/)。它允许符合许可条款的非商业使用、学习、修改与再分发；商业用途需要另行取得许可。对教育机构等组织的适用范围以正式许可条款为准。

由于限制商业用途，本项目属于 **source-available（源代码公开）**，**不是 OSI 认可的开源软件**。Limine 等第三方代码遵守各自许可证，详见 [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md)。

本项目仅供教学、研究和 AI 编程实验使用，不提供任何生产适用性、安全性或长期维护保证。GitHub Release、归档与后续维护决策由项目发起人执行。
