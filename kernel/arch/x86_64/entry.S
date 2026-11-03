/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
.section .text
.global _start
.type _start, @function
_start:
    cli                         /* No kernel IDT yet: never enable IRQs. */
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
    cli
.global kernel_idle_halt
kernel_idle_halt:
    hlt                         /* NMI/debug may resume; loop back to HLT. */
    jmp kernel_idle_halt
.size kernel_idle, .-kernel_idle

.section .bss
.balign 16
.global boot_stack_bottom
boot_stack_bottom:
    .skip 65536
.global boot_stack_top
boot_stack_top:
.section .note.GNU-stack,"",@progbits
