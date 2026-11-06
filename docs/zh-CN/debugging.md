# GDB调试

```sh
# Terminal 1
make debug
# Terminal 2
gdb -x scripts/debug.gdb
```
```gdb
hbreak *interrupt_dispatch
continue
info registers
list
stepi
p/x memory_state
p/x sched_state
p/x tasks
p/x last_exception
p/x irq_state.exception_cr2
x/8gx $cr3 + memory_state.hhdm
detach
quit
```
入口首条指令处检查RSP%16=8。CR3是物理地址，访问表前加HHDM并掩去页表标志。页故障检查vector/error/RIP/RSP与CR2：bit0权限、bit1写、bit2用户、bit4取指。参考tests/phase2.py与phase3.py独立遍历验证。不要随意修改CR3或返回帧。

[QEMU](qemu.md)

[Index](index.md)
