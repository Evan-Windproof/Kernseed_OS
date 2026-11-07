# Debugging with GDB

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
Break at the first instruction to check RSP%16=8. CR3 is physical: add HHDM before dereferencing tables, and mask page-table flags. For page faults inspect vector/error/RIP/RSP and CR2; protection bit0, write bit1, user bit2, instruction bit4 explain the access. Use tests/phase2.py and phase3.py for independent table/frame verification. Never change CR3/return frames casually.

[QEMU](qemu.md)

[Index](index.md)
