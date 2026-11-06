# Architecture

A kernel controls CPU, memory and devices. A monolithic kernel places these services in privileged kernel space. Kernseed uses separate source modules, not separate service processes. Shell is a kernel task; applications cross a hardware privilege boundary.

[Source / 源码](../../kernel/core/main.c) · [Index / 目录](index.md)

```mermaid
flowchart TD
  Limine --> entry.S --> kernel_main
  kernel_main --> GDT_TSS_IDT
  GDT_TSS_IDT --> PMM_VMM --> PIC_PIT_PS2
  PIC_PIT_PS2 --> scheduler_start --> IRQ0
  IRQ0 --> interrupt_dispatch --> scheduler_interrupt --> IRETQ
  PS2 --> keyboard_ring --> shell_task --> parser --> registry --> command
  command --> Kernel_API
  Ring3 --> INT82 --> pointer_validation --> Kernel_API
```

Kernel sources live under kernel/arch, core, drivers, interrupts, memory, scheduler and syscall; shell/commands contains independent command handlers. boot contains Limine config/linker layout, apps contains independent application sources, tests contains host and emulator checks. Shared CR3 retains kernel/HHDM mappings; only the dedicated VMM window is changed.
