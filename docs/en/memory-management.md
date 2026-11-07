# Physical and virtual memory

A physical address identifies RAM; a virtual address is translated through CR3 and four levels of 512-entry tables. HHDM supplies a kernel virtual alias of physical RAM, not ownership. PMM admits only complete USABLE 4KiB frames, rounds reservations outward, checks overflow and tracks FREE/OWNED/TABLE plus mapping references. Metadata occupies about3MiB in the kernel image. VMM owns an unused PML4 slot at 0xffffc00000000000 (1GiB window); new tables are zeroed, conflicts rejected and TLB entries invalidated. Writable/User/NX permissions accumulate across levels. Huge mappings can be queried but not split. Unmapped data frames can be freed; empty tables remain reserved.

[Source / 源码](../../kernel/memory/memory.c) · [Index / 目录](index.md)

## Practice and verification

```sh
make test
```

Observe actual guest state: memory_state / PMM exhaustion / CR3 walk / NX faults. Tests require QEMU/GDB on PATH (or QEMU/GDB overrides). Fault fixtures use separate output directories; normal boot does not inject them. Read the linked source alongside the GDB checks before changing it.
