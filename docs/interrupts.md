# x86-64 异常、中断与 PIT（Phase 2）

## 硬件范围与初始化顺序

当前支持 QEMU PC/SeaBIOS、单核 x86-64、传统双 8259 PIC 和 8254 兼容 PIT。
选择 PIT 是因为它成熟、容易观察，并不需要提前引入 ACPI/APIC 校准机制。
不声称支持任意实机、APIC/SMP 或用户态进程。沿用 Limine base revision 3 与启动页表。

入口保持 CLI/CLD → Phase 1 串口/屏幕与 CPU 自检 → GDT/TSS → IDT → PIC 全屏蔽 →
软件 INT 返回自检 → PIT 配置并仅解除 IRQ0 屏蔽 → BOOT_READY → idle 中 STI/HLT。
初始化、故障和正常 idle 分别处理；`cpu_halt` 永不启用 IRQ。

## GDT、TSS、IST

`gdt.c` 使用 5 个 8 字节槽：null、CPL0 64 位代码（selector 8）、数据（0x10）、
占两个槽的 64 位 TSS（0x18）。代码描述符 L=1/D=0，数据段 L=0；TSS 描述符不是普通数据段。
加载 GDTR 后使用 `lretq` 重载 CS，重载数据选择子并执行 `ltr`。LTR 会把 TSS 标记为 busy。
`sgdt/sidt/str` 的实际结果记录在 `irq_state`，不只记录软件希望加载的地址。

TSS 为 104 字节，RSP0 指向启动栈顶，I/O bitmap offset 超出 limit（无权限位图）。
IST1 指向独立的 16 KiB `df_stack`。IDT 向量 8 的 gate 设置 IST=1，所以 #DF 即使无法沿用
普通栈，也会先切到已映射的专用栈。其他向量当前 IST=0。
尚无页保护栈哨兵；本设计不保证在任意页表、GDT/IDT 或 IST 自身损坏时仍能可靠输出。

## IDT 与中断分类

256 项、每项 16 字节；正确拼接 handler 的 64 位地址，selector=8，reserved=0。
所有 gate 都是 present、DPL0、64 位 interrupt gate（0x8e），进入时 CPU 清 IF。

| 向量 | 来源与处理 |
|---|---|
| 0–31 | CPU 异常；保存真实现场并受控停机 |
| 32–47 | 重映射的 PIC IRQ；处理与 EOI，正常仅允许 32/IRQ0 |
| 128 | CPL0 `INT 0x80` 寄存器恢复自检；不是 syscall，不对 CPL3 开放 |
| 其他 | 非预期中断；报告并停机，不猜测恢复 |

CPU 自动压错误码的向量为 8、10、11、12、13、14、17、21、29、30。
生成脚本为其他向量补 0，再压入 vector；带错误码的异常不能再补一个 0。
**不可用 `int $14` 等指令模拟页故障**：软件 INT 不会自动压 CPU 异常错误码。

## 汇编入口与 ABI

长模式中 CPU 保存 SS/RSP/RFLAGS/CS/RIP；错误码视向量而定。统一入口再保存 15 个
通用寄存器，`struct interrupt_frame` 与汇编顺序由静态断言约束：

```text
低地址（C 参数 frame）
  r15 ... r8, rsi, rdi, rbp, rdx, rcx, rbx, rax  （15 × 8）
  vector, error                                （16 字节）
  RIP, CS, RFLAGS, 被中断 RSP, SS               （40 字节）
高地址
```

CLD 确保 C ABI 的 DF=0；原始 DF 已保存在硬件帧内。把 frame 传给 RDI，
保存真实帧指针到已备份的 RBX，向下对齐 RSP 至 16 后 call C（C 入口模 16=8）。
C 返回后恢复原帧位置，恢复所有通用寄存器，只跳过 vector/error，最后 `iretq`。
无需手动弹 RIP 或修改栈切换字段。当前只有 CPL0，无 `swapgs` 或用户地址空间切换。

编译继续使用 `-mno-red-zone -mgeneral-regs-only`；当前 ISR 不使用浮点/SIMD，也不擅自开启
嵌套硬件 IRQ。自检刻意令被中断 RSP 非 16 对齐、DF=1，并为全部通用寄存器设置哨兵，
验证 IRETQ 后数值和 DF 都恢复，再 CLD 返回普通 C。

## 异常诊断与受控停机

保存 `last_exception`：向量、错误码、通用寄存器、RIP、被中断 RSP、CS/SS/RFLAGS。
页故障在诊断输出之前读取 CR2，记录故障线性地址。错误码不解读为成功状态。
异常输出不分配内存、不获取锁、不等待 tick，不进行调度。首次致命异常标记 fatal；
诊断中再次进入异常只尝试有界串口输出，然后 CLI/HLT，避免递归 framebuffer 输出。
#DF 使用 IST 独立栈。不能把此保护描述为在硬件状态全面损坏时“绝不会 triple fault”。
所有当前异常均不可恢复，没有异常跳过、缺页按需分配或用户进程终止机制。

隔离测试使用真实指令：`div` 除零、`ud2`、超出 GDT limit 的段选择子加载、
读取未映射的 canonical 地址 `0x400000000000`。#DF 测试故意撤销 #GP gate 的 present 位，
再触发 #GP；异常投递期间的 #NP 导致真实 #DF，并通过 IST 栈地址验收。
这些指令只编入独立 TEST_FAULT 镜像，不包含在正常 ELF 中。

## PIC、PIT 与 EOI

PIC 使用 ICW1–4 重映射到 32/40、8086 模式、关闭自动 EOI。先屏蔽所有 IRQ，
最终 master mask=0xfe、slave mask=0xff，仅允许 IRQ0，键盘、RTC 与级联均不放行。
为 IRQ7/IRQ15 实现 spurious 检测：读取 ISR，spurious IRQ7 不发 EOI，spurious IRQ15
只向 master 级联发 EOI。真实 slave IRQ 才按 slave → master 顺序 EOI。
spurious 分支已实现，但当前自动验收没有注入这些时序，应与已测 IRQ0 区分。

PIT channel 0，control=0x34（低/高字节、mode 2、binary），输入时钟 1,193,182 Hz，
divisor=11,932，因此约 99.9985 Hz。tick 只在 IRQ0 handler 中递增，发送非特定 master EOI
后递增 timer_eoi 诊断计数。ISR 不打印，不把普通循环次数当时间。
64 位计数器对齐、单 CPU、interrupt gate 不允许嵌套 maskable IRQ；读取是当前配置下的快照，
不是将来 SMP 的完整同步方案。PIT 不是校准高精度时钟；没有提供完整 uptime/Shell。

## 验收边界

`make test` 先运行 Phase 1 回归，再用 GDB 检查所有 gate、GDT/TSS 与实际描述符寄存器、
每类异常 C 入口 ABI、错误码/RIP/RSP/CR2、DF IST。真实 IRQ0 帧、IRETQ 恢复 IF、
两次 tick/EOI 采样和 QEMU PIC 配置都要符合预期。单纯出现诊断文字不能通过测试。
具体命令、证据和未验证项目见 [Phase 2 报告](reports/phase-2.md)。
