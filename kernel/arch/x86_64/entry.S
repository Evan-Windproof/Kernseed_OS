/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
.section .text
.global _start
.type _start, @function
_start:
    cli                         /* IDT/PIC not ready yet: keep IF clear. */
    cld                         /* SysV C ABI requires DF clear. */
    lea boot_stack_top(%rip), %rsp
    xor %rbp, %rbp
    call kernel_main            /* RSP is 16-byte aligned before call. */
1:  cli
    hlt
    jmp 1b
.size _start, .-_start

.global kernel_idle
.type kernel_idle, @function
kernel_idle:
    sub $8, %rsp
    call scheduler_start
    add $8, %rsp
    sti                         /* Called only after GDT/IDT/PIC/PIT are ready. */
.global kernel_idle_halt
kernel_idle_halt:
    hlt                         /* IRQ0 wakes us; IRETQ restores IF=1. */
    jmp kernel_idle_halt
.size kernel_idle, .-kernel_idle

.global cpu_halt
.type cpu_halt, @function
cpu_halt:
    cli
1:  hlt
    jmp 1b
.size cpu_halt, .-cpu_halt

.section .bss
.balign 16
.global boot_stack_bottom
boot_stack_bottom:
    .skip 65536
.global boot_stack_top
boot_stack_top:
.section .note.GNU-stack,"",@progbits
