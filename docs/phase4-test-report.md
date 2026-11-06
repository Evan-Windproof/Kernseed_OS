# Phase 4 实施与测试报告

日期：2026-11-06（Asia/Taipei）。基线为最新main `ca764b0`，已合入Phase 3。
分支为codex/phase4，不重写项目、不更改宏内核架构。
开发前核对统一中断帧、PIT/EOI、PMM/VMM、构建和调度计划，原Phase 1–3完整回归通过。
实际环境：Debian 13、GCC 14.2、QEMU TCG/SeaBIOS单CPU、GDB 16.3，不是本次WSL实机验收。

## 交付实现

- 最多8个TCB（含Idle），唯一ID、名称、状态、独立栈、保存上下文、入口和真实调度统计。
- PMM连续4页/VMM可写NX任务栈，16 KiB，前后各4 KiB未映射guard，底部canary。
- 以READY槽构成有界循环就绪队列；5 tick时间片，IRQ0真正抢占，不要求任务主动yield。
- 完整GPR与RFLAGS/RIP/RSP硬件帧保存，C ABI对齐，选择目标帧后由汇编更换RSP/IRETQ。
- INT 0x81实现主动yield、按真实tick阻塞/唤醒与退出，不是用户态syscall。
- 独立Idle使用启动栈，STI/HLT；其他任务退出由不同栈延迟解除映射/释放，再复用TCB槽。
- A/B是真正的非yield汇编计数任务；C定期记录真实计数并阻塞，不并发写Framebuffer。
- 默认启动启用抢占；短任务验证yield和入口返回退出。设置demo_stop用于显式测试收尾，
  执行16轮资源复用与容量拒绝，再回到无普通READY任务的Idle。

## 实际验收与证据

完整 `make test` 执行成功：先以SCHEDULER=0执行所有旧阶段原有断言，然后重建默认
SCHEDULER=1镜像，完成抢占/回收/guard集成测试。没有删除旧内存统计或初始化检查。
额外执行延长调度测试和统计一致性复核，原始日志/结果见 [evidence/phase-4](evidence/phase-4/)。

| 要求 | 实际验证 |
|---|---|
| Phase 1–3回归 | 原宿主、ELF、64/128/256 MiB启动、描述符/异常、PIT、128/256/512/1024 MiB PMM/VMM及3类页权限故障通过 |
| 首次真实切换 | GDB断在任务A真实入口；来自IRQ0，目标RSP对齐，切换到独立PMM/VMM栈 |
| 非yield抢占 | 随后断在B入口，A计数>0且preemptions>0；A/B的yield统计始终0 |
| 完整寄存器 | A保存帧的15个GPR逐一核对非零高32位的64位哨兵，DF=1；A/B持续自检寄存器和本地栈哨兵，错误计数为0 |
| 多任务与阻塞 | A/B各自计数增长，C真实记录多次并sleep，TCB现场可观察READY/RUNNING/BLOCKED与独立上下文 |
| 主动yield/返回退出 | 短任务实际yield3次，另一个入口直接返回；退出并在别的栈上reap |
| 创建中途失败 | 已分配4页并映射2页时注入失败；检查分配/引用映射数量回滚且未发布READY |
| 单任务与复用 | 收尾时仅C拥有4页栈，执行16轮新建单个短任务、yield/退出/reap，恢复到仅C的同一资源占用 |
| TCB满额 | C再创建6个短任务占满7个普通槽，额外创建返回-1，数据页和映射数量不变 |
| 空队列/Idle | 27次真实退出与27次回收，所有已用槽TERMINATED/reclaimed，current=0，Idle IRQ开启且tick增长 |
| 无数据栈泄漏 | 结束allocated=0、mapped=0，free+tables=usable；反复idle/reap无重复释放 |
| 硬件中断整合 | IRQ0 EOI先于选择目标上下文；真实ticks与EOI一致，无其他IRQ、入口IF/DF违规或fatal |
| 上下栈边界 | 独立故障9/10在真实任务栈执行写guard，#PF vector14/error2，CR2等于对应边界，RSP仍在合法任务栈，最终IF=0/HLT |
| 统计一致性 | 活跃代TCB切换/抢占合计与全局一致，唯一RUNNING，运行tick账目与已进入IRQ0一致，PMM账目一致 |
| 重复启动/持续调度 | 128/256/512 MiB独立QEMU进程；256 MiB额外持续120秒，随后实际退出、复用和回收 |

120秒是本次实际持续验证尺度，不代表小时/天级耐久保证。计数、切换和状态来自目标CPU/内存，
非宿主模拟；QEMU/GDB暂停时不把墙钟等待伪装为运行。短回归最短允许5秒，默认测试120秒。
GDB统计采样在IRQ处理/EOI完成、调度函数入口进行，避免在一个统计更新中间读取而误判。
全局统计跨TCB复用累计；单个TCB统计属于当前任务ID那一代，收尾复用后不再直接与全局历史合计比较。

## 遇到的问题与处理

没有发现未解释的triple fault或上下文损坏。首个guard测试已通过GDB现场检查，但测试夹具
错误沿用了“启动前故障必须无BOOT_READY”的条件，导致验收脚本失败。
任务guard故障发生在正常启动和首个IRQ之后，修正为明确检查BOOT_READY存在、tick/EOI已开始后停止；
原启动前异常仍保留无BOOT_READY和tick=0的断言。原始失败日志保留，不将其报告为通过。

将寄存器哨兵加强为高32位非零的完整64位值，避免仅低位测试漏掉截断。
新增一致采样点避免观察到IRQ tick刚增、EOI未完成的瞬间。
实际不同栈回收和反复创建测试未发现数据栈泄漏；Phase 3保守保留的空页表仍单独计数。

## 代码与运行入口

新增：include/kernseed/scheduler.h、kernel/scheduler/scheduler.c、
kernel/arch/x86_64/counter-tasks.S、tests/phase4.py。
更新统一汇编返回路径、kernel_idle启动钩子、软件向量处理、构建模式及晚期故障测试夹具。
PMM/VMM接口、硬件异常帧、GDT/IDT/TSS和许可证保持原有设计。
教学说明见 [任务管理](task-management.md)、[上下文切换](context-switching.md)、[调度](scheduler.md)。

```sh
make iso
make headless                  # 或 make run
make debug                     # 另一个终端 gdb -x scripts/debug.gdb
make test                      # 全部旧回归 + 默认120秒调度测试
make test-scheduler            # 单独调度验收
KERNSEED_STRESS_SECONDS=5 make test-scheduler # 最短快速复核，不能冒充120秒测试
make BUILD=build/fault-9 TEST_FAULT=9 headless # 独立下guard故障；10为上guard
```

GDB可读取tasks、sched_state、demo_a/demo_b、register_errors；BOOT_READY是原初始化标记，
TCB在随后kernel_idle入口中、STI之前建立。最终镜像仍为build/kernseed.iso。
Git阶段提交标题为Add Kernseed OS Phase 4，分支codex/phase4；精确SHA及PR结果见交付消息。

## 已知限制

单核、最多7个普通任务、固定5 tick、无优先级、用户进程或地址空间隔离。
当前不保存FPU/SIMD/XSAVE，任务必须使用现有general-regs-only编译前提；不支持任意IRQ/NMI阻塞。
TSS.RSP0尚不跟随任务变化，因为没有CPL3；DF仍使用独立IST。
状态字节PMM和保守页表保留策略沿用Phase 3，数据栈回收不等于完整页表回收。
没有外部kill、通用锁等待队列、参数传递或shell；名称指针必须由可信调用者保持有效。
任务栈guard测试是显式访问边界，不是对任意损坏栈/页表的全面防护证明。
未在WSL、UEFI、KVM、实机或SMP上验收，不把本阶段称为完整可日用OS。

本次120秒采样实值：switches=3606、preemptions=2400、records=1200，错误计数0。
收尾后exits=reaped=27，recycle_passed=16，create_failures=2，allocated=mapped=0，
tables=4（含Phase 3保守保留的表页）。这些数值是该次日志读数，不是下次运行的固定输出。
