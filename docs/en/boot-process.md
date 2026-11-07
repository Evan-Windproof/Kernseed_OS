# Boot process

Limine loads ELF PT_LOAD segments at 0xffffffff80000000 and provides framebuffer, memmap and HHDM responses (base revision 3). entry.S executes CLI/CLD, installs a 64 KiB stack and calls kernel_main with SysV alignment. BSS/runtime checks precede GDT/TSS/IDT, memory selftests, PIC/PIT and keyboard setup. kernel_idle creates tasks before STI. BOOT_READY precedes the actual Shell prompt.

[Source / 源码](../../kernel/arch/x86_64/entry.S) · [Index / 目录](index.md)

## Practice and verification

```sh
make test
```

Observe actual guest state: boot_state / BSS / CR0 / CR3 / CR4 / EFER. Tests require QEMU/GDB on PATH (or QEMU/GDB overrides). Fault fixtures use separate output directories; normal boot does not inject them. Read the linked source alongside the GDB checks before changing it.
