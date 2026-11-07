# System call ABI

INT0x82 is a DPL3 interrupt gate rather than a normal function call. RAX carries number/result, RDI/RSI arguments. Interrupt gates switch through TSS to the kernel stack, save registers and clear IF; return uses IRETQ.

| RAX | Operation | Arguments / result |
|---|---|---|
| 0 | write | RDI=pointer, RSI=length (max256); requested length or -14 |
| 1 | uptime | actual PIT ticks, not seconds |
| 2 | yield | no arguments; returns0 |
| 3 | exit | no return |

Unknown numbers return -38. write checks overflow, complete range, User permissions at every table level and expected physical ownership, then copies to a kernel buffer. Embedded NUL ends display although requested length is returned; not POSIX binary write. Internal kernel APIs are api_* and task_*; Shell handlers are not syscall implementations.
[ABI source](../../apps/abi.h) · [Ring3](user-mode.md)

[Index](index.md)
