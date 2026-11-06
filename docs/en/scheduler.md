# Tasks and preemption

A task is an execution context, not necessarily a protected process. Eight TCB slots include Idle; ordinary tasks receive independent four-page NX kernel stacks with unmapped guards and canaries. READY becomes RUNNING; yield/expiry returns READY, sleep becomes BLOCKED until wake_tick, exit becomes TERMINATED. IRQ0 expires a five-tick quantum and returns another saved frame through isr_common. INT0x81 is an internal scheduling trap. Reaping runs on a different stack before freeing a terminated stack. A/B never yield and check full-width registers/DF/local stack sentinels; C records progress. Idle uses STI/HLT only when no ordinary task is ready. Default busy demonstrations consume CPU.

[Source / 源码](../../kernel/scheduler/scheduler.c) · [Index / 目录](index.md)

## Practice and verification

```sh
make test-scheduler
```

Observe actual guest state: tasks / sched_state / register_errors / stack guards. Tests require QEMU/GDB on PATH (or QEMU/GDB overrides). Fault fixtures use separate output directories; normal boot does not inject them. Read the linked source alongside the GDB checks before changing it.
