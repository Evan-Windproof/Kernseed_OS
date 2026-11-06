# 最小 Ring 3 环境

新增GDT用户代码/数据段，CS=0x2b、SS=0x33。保持TSS索引0x18，调度切换时更新RSP0为任务独立内核栈顶。
user_enter清空通用寄存器，构建IF=1/IOPL=0的IRETQ帧，进入独立用户代码与用户栈。
所有硬件/软件入口仍使用Phase 2完整寄存器帧；CPU从CPL3自动切到TSS内核栈，汇编再对齐调用C。

用户页位于独立受控窗口USER_CODE=ffffc00001000000、USER_STACK=ffffc00001100000。
代码一页User/RX，栈一页User/RW/NX；通过自有子树给祖先User位，但其他叶子的Supervisor位不改变。
内核、HHDM、任务内核栈仍为Supervisor。用户不能通过普通加载/存储访问它们。
这是共享CR3中的**单个活跃用户任务**，不是多个用户进程之间的完整地址空间隔离。

用户异常终止当前任务，不把CPL3的GP/PF当成内核panic。恶意用户RSP返回或被IRQ捕获时也终止，
而不是触发调度器的内核栈断言。NT等危险返回标志被过滤。内核态故障仍按原Phase 2受控停机。
数据页在其他任务栈上reap时解除映射/释放；中间空表延续Phase 3保留策略。
无TLS/FPU/SIMD、信号、SMEP/SMAP/UMIP完整防护、用户缺页恢复、外部kill或多CPU。
共享内核仍是教学实现，不声称生产安全边界或抗侧信道能力。

Phase 7：user_run 在单核 CLI 临界区完成空闲检查、分配、映射和 TCB 发布；所有返回恢复原 IF。拒绝 NULL 名称、调度未启动和 Idle 调用。当前同步 Shell 不提供并发 run。
