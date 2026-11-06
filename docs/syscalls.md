# Kernseed native syscall ABI v1

真实入口为DPL3 interrupt gate `int 0x82`，以IRETQ返回。不是内核函数调用，也不是Linux ABI。
RAX为编号/有符号返回值，RDI/RSI为参数；保存其他通用寄存器。允许的用户返回标志仅算术标志/DF，
强制IF=1、IOPL=0，移除NT/VM/TF等危险控制位；入口另外清除当前NT，避免内核IRET受其影响。

| 编号 | 操作 | 参数/结果 |
|---|---|---|
| 0 | write | RDI=地址，RSI=长度（0..256），验证整段后输出文本，返回长度；EFAULT=-14 |
| 1 | uptime | 返回真实PIT tick，不是墙钟或纳秒 |
| 2 | yield | 内核调度API主动让出，恢复后返回0 |
| 3 | exit | 结束当前任务，不返回 |

未知编号返回ENOSYS=-38。零长度write不解引用地址；文本含NUL时显示在NUL终止，非POSIX字节流保证。
copy_user检查溢出、受管用户代码/栈范围、每页Present/User权限及对应物理页所有权，再经HHDM复制。
单CPU、入口IF=0、一个活跃用户任务、无用户映射修改接口，避免检查和复制之间的用户并发撤销。
内核指针、未映射或跨界地址拒绝，不直接解引用用户传入指针。

Kernel API见include/kernseed/api.h：终端、真实内存/TCB快照、tick、退出；任务创建/yield/sleep沿用scheduler接口。
Shell命令使用内核API，不作为syscall实现。syscall内可以嵌套0x81调度，返回原内核栈后再IRET到用户。
