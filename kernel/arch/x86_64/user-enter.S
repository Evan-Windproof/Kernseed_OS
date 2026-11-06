/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
.text
.global user_enter
.type user_enter,@function
user_enter:
    mov $0x33,%ax
    mov %ax,%ds
    mov %ax,%es
    pushq $0x33
    push %rsi
    pushq $0x202
    pushq $0x2b
    push %rdi
    xor %eax,%eax
    xor %ebx,%ebx
    xor %ecx,%ecx
    xor %edx,%edx
    xor %ebp,%ebp
    xor %edi,%edi
    xor %esi,%esi
    xor %r8,%r8
    xor %r9,%r9
    xor %r10,%r10
    xor %r11,%r11
    xor %r12,%r12
    xor %r13,%r13
    xor %r14,%r14
    xor %r15,%r15
    iretq
.section .note.GNU-stack,"",@progbits
