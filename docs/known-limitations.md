# 已知限制（Phase 7 功能冻结）

- 单核 x86-64、PC/AT PIC/PIT、i8042 US Set 1、COM1 和 Limine framebuffer。没有 APIC/SMP、USB、磁盘文件系统、网络、POSIX 或 Linux ABI。
- 最多 8 个 TCB（含 Idle）；默认 A/B/C/Shell 占用槽和独立栈，RR 固定 5 tick。任务内核栈 16 KiB，前后未映射 guard；DF 16 KiB IST。没有通用锁等待队列、外部 kill 或优先级。
- 不保存 FPU/SIMD/XSAVE/TLS 状态；必须沿用 general-regs-only 编译前提。可信内核调用者负责名称、指针和物理页生命周期；禁用于 NMI/SMP 分配。
- PMM 只纳入完整 USABLE 页；最多 256 内存区域、物理可用 RAM 低于 4 GiB。静态状态/引用元数据约 3 MiB。扫描分配最坏线性，不是常数时间 bitmap 搜索。
- 沿用 Limine 四级 CR3/HHDM；需空的 PML4 槽，受控 VMM 窗口 1 GiB。只新建/解除 4 KiB 页；可查询大页，不拆分或修改继承大页。拒绝 LA57/PCID 等未支持模式。
- 中间页表 TABLE 保守保留（1 GiB 区域最多 514 页），失败映射也可能留下空表。应用/任务数据页可回收，不代表完整地址空间/表页回收。HHDM 永久别名没有 OWNED 引用，不能引用已释放页。
- 同时仅一个活跃 Ring3 任务，共享 CR3 固定受 User 权限控制窗口；代码/栈各 4 KiB。可阻止用户访问 supervisor 内核映射，不提供多个恶意用户进程间的隔离。
- **没有通用 ELF 加载器**。应用独立编译成 ELF 后转固定平坦映像嵌入；内核只接纳白名单和大小检查。无 ELF 头/PT_LOAD 解析、多段/BSS 加载、重定位、动态链接或任意文件执行。
- 自有 INT 0x82 ABI，仅 write/uptime/yield/exit。write 最长 256 字节，拷入内核后按文本 NUL 结尾显示，返回请求长度；不是二进制/POSIX write。tick 按约 100 Hz PIT 计数，屏蔽中断会影响计时，不能当高精度墙钟。
- run 同步等待，无 Ctrl-C/kill；无限运行的应用可能永久占用 Shell 的 run，但不会因此取消 PIT 抢占。可从 QEMU 终止/调试。
- 用户态异常终止当前任务；内核异常和资源不变量失败受控停机。guard 不能挽救任意损坏的内核栈/页表；无全面故障恢复与生产加固。
- 终端单所有者、有限字体/ASCII，字形大写但输入区分大小写；无 Unicode、完整 VT、历史、管道或脚本。行最多 127 字节、8 token、键盘队列 255 字符，溢出拒绝截断执行，可能需再次 Enter。
- UART 只有输出，make headless 的 stdin 不能操作 Shell。使用 QEMU 图形键盘或 QMP 的真实键盘事件。默认忙任务持续演示，不是整机空闲低功耗模式。
- 没有生产环境安全性、实时性能或长期维护保证；此次有限测试不是全面安全审计。
- 云端实测 Debian/QEMU TCG；WSL 2 Phase1–6 启动是发起人报告，本次没有在 WSL/VMware/KVM/真实硬件执行 Phase7。UEFI 仅覆盖报告指定的 OVMF、无 Secure Boot；不泛化到所有固件。

许可证是 PolyForm Noncommercial 1.0.0（source-available，非 OSI Approved）。第三方组件保留原许可。
