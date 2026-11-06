# Ring3 execution

user_enter constructs CS=0x2b/SS=0x33 and IRETQ enters Ring3 with a separate user stack. Code at0xffffc00001000000 is one RX User page; stack at0xffffc00001100000 is one RW/NX User page. Supervisor kernel/HHDM mappings stay inaccessible. All tasks share CR3 and only one user image may be active. TSS.RSP0 switches to the task kernel stack on a gate entry. Bad user flags/RSP are checked; faults record RIP/error/CR2 and terminate only the user task. Reaping unmaps both user pages and the kernel stack. This is not multi-process address-space isolation.

[Source / 源码](../../kernel/syscall/user.c) · [Index / 目录](index.md)

## Practice and verification

```sh
make test-integration
```

Observe actual guest state: CS / SS / TSS.RSP0 / user_state / page permissions. Tests require QEMU/GDB on PATH (or QEMU/GDB overrides). Fault fixtures use separate output directories; normal boot does not inject them. Read the linked source alongside the GDB checks before changing it.
