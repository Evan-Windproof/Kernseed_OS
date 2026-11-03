# Kernseed OS

独立实现的教学用 x86-64 宏内核，以 C 为主、少量汇编，使用 Limine 引导。
它不是 Linux 发行版，也不依赖宿主用户态服务。项目与 Orfreon、Evavind 无关。

**当前范围：Phase 1 引导。** 已实现独立入口、64 KiB 启动栈、基本 C 内存函数、
COM1 串口、framebuffer 文本显示、启动自检和 HLT 主循环。
GDT/IDT、中断、内存分配器、调度器、键盘、Shell 和系统调用均尚未实现。
目前不能输入命令；内核正常启动后停在 HLT 是预期行为。

项目是 **source-available**，自有源码采用 [PolyForm Noncommercial 1.0.0](LICENSE)，
不应称为 OSI Approved Open Source。不面向商业使用，不保证生产环境安全性，
不承诺长期维护。发布与后续归档由发起人最终审查和执行。

## 构建环境

推荐 x86-64 WSL 2 Ubuntu 24.04+ 或 x86-64 Debian/Ubuntu。
本阶段实际验证平台为 Debian 13 云环境，**尚未在 WSL 2 上实测**。
需要 GCC（支持 `-m64 -mcmodel=kernel`）、GNU ld、Make、Git、Python 3、
QEMU、GDB、xorriso；宿主 libc 仅用于构建工具和宿主测试，不链接入内核。

```sh
sudo sh scripts/setup-host.sh
make deps             # HTTPS 获取固定提交的 Limine，并核对 SHA-256
make -j2             # build/kernseed.elf 和 build/kernseed.iso
make test            # 宿主测试、ELF 检查、实际 QEMU/QMP/GDB 测试
make reproducible    # 同路径、同工具链，两次强制重建后比较 ELF/ISO SHA-256
```

`make` 也会自动准备 Limine。首次联网仅需要官方 GitHub；系统工具从发行版签名软件源安装。
依赖下载到 `.cache/`，产物在 `build/`，两者均忽略，不自动提交。
`CC`、`LD`、`HOST_CC`、`XORRISO` 可用 Make 命令行覆盖；`QEMU`、`GDB` 是程序路径环境变量。
只有 GNU 风格 x86-64 工具链经过验证，未声称支持任意编译器。
ISO 固定文件日期与 GPT GUID 以避免随机构建元数据；不承诺不同绝对路径、工具版本间逐字节相同。
可在忽略的 `config.local.mk` 中配置本地工具路径。当前云实例通过该文件使用工作区内的
QEMU/GDB/xorriso，无需 root 或修改系统目录；它不是可移植的项目依赖声明。

```sh
make run             # GTK 显示窗口 + 当前终端的串口日志
make headless        # 无窗口，仅串口；Ctrl-C 结束
make debug           # 启动时暂停，GDB 仅监听 127.0.0.1:1234
# 在第二个终端、仓库根目录：
gdb -x scripts/debug.gdb
```

无图形后端的云环境使用 `make headless` 和 `make test`。
桌面可通过 `QEMU_DISPLAY=sdl make run` 选择宿主已安装的显示后端。
WSLg/图形后端问题见 [QEMU 指南](docs/qemu-guide.md)。
ISO 包含 BIOS 和 x86-64 UEFI 引导文件，目前自动验收限定 **SeaBIOS + TCG + 单核**。
UEFI、实机和 KVM 尚未验证。

## 验证与证据

`make test` 真正启动 64/128/256 MiB 三种内存的 QEMU，捕获屏幕并检查字形像素，
通过 GDB 检查 C 入口 ABI、BSS、长模式、CR3 和启动状态，另外测试无 framebuffer 的 panic。
宿主测试对 12,464 种重叠复制组合、边界哨兵和 framebuffer 边界进行检查。
每次运行的日志、截图、命令和结果保存在 `build/test-results/<UTC时间>/`。
失败返回非零；编译通过和串口文字不能单独证明全部功能正确。

本阶段报告见 [Phase 1](docs/reports/phase-1.md)。后续阶段的验收尚未执行。

## 结构与学习路线

| 目录 | 当前职责 |
|---|---|
| `boot/` | Limine 配置、ELF 链接布局 |
| `kernel/arch/x86_64/` | CPU 入口、栈、HLT 循环 |
| `kernel/core/` | 启动流程、C 内存函数、输出组合接口 |
| `kernel/drivers/` | 轮询串口、framebuffer 文本 |
| `include/kernseed/` | 内核内部接口 |
| `third_party/limine/` | 带原许可的协议头、固定依赖校验清单 |
| `scripts/`, `tests/`, `docs/` | 构建运行、自动测试、教学说明 |

后续模块在进入对应阶段时建立，避免空目录伪装成实现。

- [模块依赖与阶段计划](docs/architecture.md)
- [启动流程](docs/boot-process.md)
- [内存管理现状与计划](docs/memory-management.md)
- [中断现状与计划](docs/interrupts.md)
- [任务调度计划](docs/scheduler.md)
- [QEMU 操作](docs/qemu-guide.md)、[GDB 操作](docs/gdb-guide.md)
- [Codex 实验记录](docs/codex-experiment.md)
- [第三方许可](THIRD_PARTY_LICENSES.md)

开发阶段不自动 push、发布版本或归档仓库；远程操作需要发起人明确授权。
