# GDB 调试指南

在两个终端中均切到仓库根目录：

```sh
# 终端一
make debug
# 终端二
gdb -x scripts/debug.gdb
```

脚本加载带 DWARF 符号的 `build/kernseed.elf`，连接 `127.0.0.1:1234`，
设置 `hbreak *kernel_main` 并继续。星号让断点落在第一条机器指令，而不是 C 行号对应的
函数序言之后；这是验证入口栈对齐的前提。硬件断点避免在尚未映射的内核地址写入软件断点。

```gdb
info registers rip rsp eflags cr0 cr3 cr4
p/x (unsigned long)$rsp & 15
x/8gx $rsp
delete breakpoints
hbreak kernel_idle
continue
p/x boot_state
x/8i kernel_idle
```

C 入口 RSP 模 16 应为 8，DF/IF 均为 0。idle 前 `boot_state.stage` 应为 2，
`runtime_result` 为 0，`bss_ok` 和 `serial_ok` 为 1。
`cr0` 含 PG/PE，`cr4` 含 PAE，`efer` 含 LME/LMA，`cr3` 是实际启动页表物理基址。
`boot_state` 字段由硬件读取或执行测试产生，不是手填状态值。

`kernel_idle_halt` 是 HLT 指令位置；在 HLT 单步可能一直等中断，宜在执行前断住，
或 Ctrl-C 暂停后检查。Phase 2 的 IRQ0 会周期性唤醒 HLT；但 GDB 暂停整个 vCPU 时，tick 不会增长。

`make test` 使用独立 UNIX GDB socket 自动验证上述条件，保存 `gdb.log` 和执行脚本。
宿主 GDB 本身的动态库、XML/符号解析等错误不应视作内核失败，也不能忽略。
调试端口具备完全控制虚拟机的能力，保持仅监听本机，不对外暴露。

## Phase 2 描述符、中断与定时器

```gdb
hbreak kernel_idle
continue
p/x irq_state
p/x gdt
p kernel_tss
p/x idt[8]
hbreak kernel_idle_halt
continue
p irq_state.ticks
continue
p irq_state.ticks
p irq_state.timer_eoi
monitor info pic
```

`kernel_idle` 入口尚未 STI；`kernel_idle_halt` 已 IF=1。每次 `continue` 允许真实 IRQ
发生后再断住；不要在暂停状态反复读取 tick 并期望它自行增长。
GDTR.limit=39，IDTR.limit=4095，TR=0x18；IDT[8].ist=1，TSS.ist[0] 是 DF 栈顶。
`hbreak *interrupt_dispatch` 可在第一条指令查看 `$rdi` 指向的 `struct interrupt_frame`，
验证 C 入口 RSP 模 16=8、IF/DF=0，并区分向量 128 的内部软件自检与向量 32 的硬件 IRQ0。
该软件向量不对 CPL3 开放，不是系统调用接口。

危险异常请使用 `make test-phase2` 自动创建的独立镜像与匹配 ELF，不要在正常构建手动
执行 `int $8` 等错误码向量。`int` 不会模拟 CPU 的错误码压栈，也不等同于真实异常。
