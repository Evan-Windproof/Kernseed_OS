# Phase 7 稳定性审查

基线 main `7c3c921`（合入 Phase 6），分支 `codex/phase7`。本次是有限范围的源码检查与运行验证，
不是全面安全审计。严重级别：Critical 破坏内核/权限边界；High 内存损坏/死锁/严重功能错误；
Medium 稳定性/模块正确性；Low 文档、风格或非关键改进。

## 发现及处理

| 编号 | 严重级别 | 证据性质 | 发现、处理和验证边界 |
|---|---|---|---|
| S7-01 | High | 静态推断，未复现并发故障 | `user_run` 检查 active 后，在未屏蔽抢占的区间分配并写全局 code/stack 页变量；两个可信内核调用者可能交错。现在从检查、分配、映射到发布/回滚统一保存 IF/CLI，拒绝 NULL、调度未启动和 Idle 调用；嵌套锁保持 IF。当前 Shell 原来就是串行调用，应用测试验证正常生命周期与拒绝未知应用，不冒称复现并发竞态。仅适用单核。 |
| S7-02 | Medium | 静态语言规则 | 链接器给出的 start/end 不是 C 同一数组对象，原比较/指针相减不满足 C 规则。改为 uintptr_t 数值边界检查、差值后复制；保留 4096 字节上限。应用独立构建与真实执行覆盖实际映像。 |
| S7-03 | Medium | 静态构建依赖 | 应用 ELF 和生成 apps.S 未依赖 Makefile，修改应用集合/标志可能保留旧输出。补 Makefile 前置依赖；增量与两次干净构建验证。 |
| S7-04 | Medium | 静态诊断缺口、隔离运行验证 | 调度器约束失败和用户回收失败原来静默停机。补有界串口输出；调度器先 CLI。新测试在独立 QEMU 中损坏任务栈 canary，由真实 IRQ 路径检测并受控停机，GDB 验证错误计数和 IF=0。用户回收失败分支只静态检查，未强行破坏页表以触发。 |
| S7-05 | Low | 静态文档/界面 | about、架构图、README 部分仍声称没有用户态；PIT 日志称仅 IRQ0，但 Shell 启用 IRQ1。必要同步到真实实现与 Phase 7。历史阶段报告保留原阶段语境。 |
| S7-06 | Medium | 测试方法 | 原 reproducible 使用 make -B，不是干净构建。现在在专用固定输出路径两次 clean 后构建，保留正常镜像与回归日志；两次哈希由实际命令生成。 |

没有在此次测试中发现未处理的 Critical/High 故障。这不是不存在其他缺陷的证明。

## 核心代码、依赖和覆盖清单

| 模块/源码 | 静态核对 | 实际测试 |
|---|---|---|
| `entry.S`、`main.c`、`runtime.c`、链接脚本、Limine 请求 | CLI/CLD、64 KiB 栈、SysV 对齐、BSS、请求段、RX/RW 段；初始化顺序保持先描述符和内存、后 PIT/任务、最后 STI | C 宿主自检、ELF、冷启动、真实 CPU/页表检查；无 framebuffer 受控失败 |
| `gdt.c`、`gdt-load.S`、`interrupts.c`、`gen-isr.py`、`isr-common.S`、`interrupt-probe.S` | TSS 双槽和 I/O bitmap 禁止用户 I/O；DF IST；错误码集合；15 GPR、CLD、NT、C 对齐、目标 RSP/IRETQ；PIC 屏蔽/EOI | GDB 检查全部 IDT/GDT、CPU 真 #DE/#UD/#GP/#PF/#DF、IRQ0 计数、软件 INT、入口 IF/DF 和寄存器恢复 |
| `memory.c`、`span.c` | usable 内缩、保留区外扩、溢出、元数据保护；OWNED/TABLE/ref；页表清零、冲突拒绝、INVLPG、受控窗口；HHDM 只作地址转换 | 四 RAM 配置实际穷尽/恢复、PMM 逐页账目、CR3 遍历、读写/解除、只读/NX 故障；继承内核页权限 |
| `scheduler.c`、`counter-tasks.S` | 独立 NX 栈/guards/canary，嵌套 IF 锁；READY 扫描、BLOCKED 唤醒、延迟回收、TSS.RSP0 | 非 yield A/B 高 32 位 GPR/DF/栈哨兵、120 秒连续调度、统计一致、27 次退出/回收、创建失败回滚、空队列 Idle、guard PF |
| `user.c`、`user-enter.S`、`api.c`、`apps/*` | 真 CPL 边界、独立用户/内核栈，指针全范围/每级权限/物理归属，长度限制，返回 flags/RSP 验证，回收所有权 | GDB 实际 CS/SS/RSP、int82 入口、Ring3 抢占；坏指针/编号、NT/RSP、特权指令、内核读写 PF；重复退出与资源账目 |
| `keyboard.c`、`terminal.c`、`framebuffer.c`、`serial.c` | 有限轮询、IRQ 不分配/显示；255 字符队列、溢出丢行；pitch/格式/滚屏边界、UART 超时 | 真实 PS/2 键入、FIFO 背压/恢复、Shift/退格、截图像素、滚屏宿主哨兵、i8042 reboot |
| `shell/*`、独立 `commands/*` | parser/registry/dispatch/backend 分离，127 字节/8 token；统一描述符/参数范围；help 遍历 registry | 空/未知/引号/转义/超限输入，全部十条命令；独立 probe 注册无需改核心；多轮 apps/echo |
| Makefile、构建/运行/调试脚本、第三方清单 | 依赖、freestanding/no-red-zone/general-regs-only，固定 Limine 提交和 SHA256，脚本执行位，无个人路径提交 | 干净输出构建/哈希、故意编译失败非零、源码指纹不变、QEMU/GDB 工作流 |

上述测试具体次数、时长及结果以 phase7-test-report.md 和原始证据为准。并非每个静态判断都有单独动态故障注入。

## 保留的设计边界

- 默认 A/B 是教学用忙任务，所以系统空输入时也会持续消耗一个模拟 CPU。不将默认忙循环冒称低功耗 Idle；无就绪任务的独立测试确认 STI/HLT 可被 IRQ 唤醒。
- PMM 扫描和页表/映像创建期间关闭中断，最坏延迟尚未测定；没有为性能移除安全检查。
- VMM 空中间表有意保留，首次访问不同窗口会增加 tables，不属于应用页/栈泄漏；重复相同窗口后应稳定。
- 任意可信内核任务可直接访问 HHDM，API 指针和任务名依赖调用者生命周期；不是恶意内核模块隔离系统。
- 没有外部 kill。内置同步 run 若程序不退出，Shell 等待，但 PIT 和其他任务仍可运行；本阶段不添加进程管理框架。
- 单活跃 Ring3/shared CR3、无 FPU/TLS/SMP、固定映像等限制见 known-limitations.md。
