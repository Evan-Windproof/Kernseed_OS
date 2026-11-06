# 中断帧驱动的真实上下文切换

沿用Phase 2统一帧：15个通用寄存器、vector/error、RIP/CS/RFLAGS/RSP/SS，共176字节。
入口保存全部GPR，CLD，向下对齐RSP后调用interrupt_dispatch。后者处理设备与EOI，
再由scheduler_interrupt选取返回帧。汇编将RAX返回的帧地址装入真正的RSP，恢复寄存器并IRETQ。
没有普通函数轮流调用任务，也没有在C中伪造“运行结果”。

IRQ0在时间片到期时可以选择其他栈；软件INT 0x81用于主动yield/sleep/exit。
软件API先保存调用者RFLAGS、CLI并设置请求，再INT；该任务恢复后才恢复原始IF。
因此切换不会在修改队列或恢复寄存器一半时提前开放IRQ。
0x81是DPL0内核内部接口，不是用户态syscall；Phase 2的0x80寄存器自检保持原义。

首次任务上下文位于新栈上，CS=8/SS=0x10，RFLAGS=0x202，RSP=栈顶-8，符合SysV入口要求。
RIP指向trampoline，调用TCB入口；入口返回自动task_exit。后续保存的帧完全来自CPU实际中断。
C调度函数始终在旧栈上返回完毕，再由汇编切RSP，不能在仍使用旧栈的C函数中释放旧栈。

编译禁用red zone与浮点/SIMD。GDT、页表和地址空间不随内核任务切换；不保存XSAVE或用户GS。
任务A/B的汇编循环保留寄存器与本地栈哨兵，设置DF=1、不调用yield；IRQ入口必须CLD并在IRETQ
恢复DF。任何哨兵损坏都会增加错误计数并停机，不靠宿主模拟任务执行。
