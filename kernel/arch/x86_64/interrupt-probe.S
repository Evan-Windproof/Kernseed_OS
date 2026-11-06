/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
.section .text
.global interrupt_register_probe
.type interrupt_register_probe, @function
interrupt_register_probe:
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    /* Deliberately non-16-aligned interrupted RSP and DF=1. The ISR must
     * realign for C, clear DF there, and restore original RSP/flags by IRETQ. */
    mov $0x101, %rax
    mov $0x102, %rbx
    mov $0x103, %rcx
    mov $0x104, %rdx
    mov $0x105, %rbp
    mov $0x106, %rdi
    mov $0x107, %rsi
    mov $0x108, %r8
    mov $0x109, %r9
    mov $0x10a, %r10
    mov $0x10b, %r11
    mov $0x10c, %r12
    mov $0x10d, %r13
    mov $0x10e, %r14
    mov $0x10f, %r15
    std
    int $0x80
    cmp $0x101, %rax
    jne 2f
    cmp $0x102, %rbx
    jne 2f
    cmp $0x103, %rcx
    jne 2f
    cmp $0x104, %rdx
    jne 2f
    cmp $0x105, %rbp
    jne 2f
    cmp $0x106, %rdi
    jne 2f
    cmp $0x107, %rsi
    jne 2f
    cmp $0x108, %r8
    jne 2f
    cmp $0x109, %r9
    jne 2f
    cmp $0x10a, %r10
    jne 2f
    cmp $0x10b, %r11
    jne 2f
    cmp $0x10c, %r12
    jne 2f
    cmp $0x10d, %r13
    jne 2f
    cmp $0x10e, %r14
    jne 2f
    cmp $0x10f, %r15
    jne 2f
    pushfq
    pop %rax
    test $0x400, %rax
    jz 2f
    xor %eax, %eax
    jmp 3f
2:  mov $1, %eax
3:  cld
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret
.size interrupt_register_probe, .-interrupt_register_probe

/* Real faulting instructions, called ONLY by isolated TEST_FAULT builds. */
#if KERNSEED_TEST_FAULT > 0
.global fault_de, fault_ud, fault_gp, fault_pf
.global fault_de_rip, fault_ud_rip, fault_gp_rip, fault_pf_rip
fault_de:
    mov $1, %eax
    xor %edx, %edx
    xor %ecx, %ecx
fault_de_rip:
    div %rcx
    ret
fault_ud:
fault_ud_rip:
    ud2
    ret
fault_gp:
    mov $0x28, %eax             /* Beyond our 5-slot GDT: #GP(0x28). */
fault_gp_rip:
    mov %ax, %ds
    ret
fault_pf:
    mov $0x0000400000000000, %rax /* Canonical, outside Limine mappings. */
fault_pf_rip:
    mov (%rax), %rax
    ret
#endif
.section .note.GNU-stack,"",@progbits
