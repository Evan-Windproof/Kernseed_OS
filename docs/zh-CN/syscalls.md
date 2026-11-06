# 系统调用ABI

INT0x82是DPL3中断门，不是普通函数调用。RAX传编号/结果，RDI/RSI传参数。CPU经TSS切到内核栈，保存寄存器并清IF，最后IRETQ返回。

| RAX | Operation | Arguments / result |
|---|---|---|
| 0 | write | RDI=pointer, RSI=length (max256); requested length or -14 |
| 1 | uptime | actual PIT ticks, not seconds |
| 2 | yield | no arguments; returns0 |
| 3 | exit | no return |

未知编号返回-38。write检查长度/溢出、完整范围、各级User权限与物理页归属，再复制到内核缓冲。NUL会终止显示但返回请求长度，不是POSIX二进制write。内核内部服务为api_*与task_*，Shell handler不充当系统调用实现。
[ABI source](../../apps/abi.h) · [Ring3](user-mode.md)

[Index](index.md)
