# 内核任务管理（Phase 4）

内核任务共享同一个 CPL0 地址空间，不是具有独立页表、用户权限或进程隔离的用户进程。
当前最多8个TCB，含ID为0的Idle。TCB保存单调增长ID、名称、状态、入口、物理栈页、虚拟边界、
保存的中断帧指针，以及切换/抢占/运行tick/yield统计。名称由调用者保持有效，不复制字符串。

状态：UNUSED → READY → RUNNING；RUNNING因时间片到期/yield回到READY，sleep进入BLOCKED，
到达wake_tick后回READY；入口返回或task_exit进入TERMINATED。终止且已回收的槽可以复用，
新任务获得新ID。当前不提供任意外部kill或锁等待队列。

任务栈由PMM分配连续4页，经VMM映射成16 KiB可写NX区，前后各保留一页未映射guard。
创建先完成页分配、映射、清零、canary和首次IRETQ帧，最后发布READY状态。
失败时解除已建立映射并释放所有新数据页，不发布半成品TCB。
Phase 3的空中间页表保守保留并计数，不能把一次性增加的页表误称为数据栈泄漏。

Idle独立使用64 KiB启动栈；其他任务均使用自己的PMM/VMM栈，不共享调用栈。
保存的context指针只在任务离开CPU后或初次启动前有意义；运行期间栈可能覆盖上一次旧帧，
直到下一次中断重新保存。TSS.RSP0仍指启动栈，因为当前没有CPL3切换；DF继续使用独立IST。

退出不在当前栈释放页。调度器先把它标记TERMINATED并切走，下一次运行于其他任务/Idle栈的
调度入口才reap；解除映射、INVLPG后再释放PMM页，清空context/physical并标记reclaimed。
重复进入reap不会重复释放。待回收任务不进入就绪集合。

API限正常任务上下文、单核；不能从NMI、异常处理或任意IRQ调用yield/sleep/exit。
PMM/VMM和任务队列操作保持CLI临界区，不在锁持有期间阻塞。无SMP或FPU/SIMD上下文支持；
内核任务必须继续使用现有-mgeneral-regs-only/-mno-red-zone构建配置。
