# Interrupts and exceptions

GDT defines kernel/user segments and a two-slot TSS. TSS.RSP0 follows the current kernel stack; Double Fault uses a separate 16 KiB IST. IDT has 256 gates; only INT 0x82 is DPL3. Assembly normalizes hardware error codes, saves 15 GPRs, clears DF/NT, aligns the C stack, then restores the selected frame with IRETQ. PIC maps IRQ0/1 to vectors32/33; other devices remain masked. PIT divisor11932 gives about100Hz; EOI precedes scheduling. Kernel faults halt; user faults terminate the task.

[Source / 源码](../../kernel/arch/x86_64/isr-common.S) · [Index / 目录](index.md)

## Practice and verification

```sh
make test-phase2
```

Observe actual guest state: irq_state / last_exception / DF IST / ABI / EOI. Tests require QEMU/GDB on PATH (or QEMU/GDB overrides). Fault fixtures use separate output directories; normal boot does not inject them. Read the linked source alongside the GDB checks before changing it.
