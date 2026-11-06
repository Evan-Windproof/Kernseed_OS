/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
.section .text
.global gdt_load
.type gdt_load, @function
gdt_load:
    lgdt (%rdi)
    pushq $0x08
    lea 1f(%rip), %rax
    pushq %rax
    lretq
1:  mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs
    mov $0x18, %ax
    ltr %ax
    ret
.size gdt_load, .-gdt_load
.section .note.GNU-stack,"",@progbits
