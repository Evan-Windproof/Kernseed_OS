/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
.section .text
.global isr_common
.type isr_common, @function
isr_common:
    /* Interrupt gates clear IF. Preserve ALL general-purpose registers.
     * Hardware saved RFLAGS, so CLD does not lose the interrupted DF. */
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rbp
    push %rdi
    push %rsi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    cld
    mov %rsp, %rdi
    mov %rsp, %rbx               /* C preserves RBX; original RBX is on stack. */
    and $-16, %rsp              /* SysV: aligned BEFORE call. */
    call interrupt_dispatch
    mov %rbx, %rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rsi
    pop %rdi
    pop %rbp
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    add $16, %rsp              /* Remove normalized vector and error only. */
    iretq
.size isr_common, .-isr_common
.section .note.GNU-stack,"",@progbits
