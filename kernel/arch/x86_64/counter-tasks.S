/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
.section .text
.global counter_task_a
.type counter_task_a, @function
counter_task_a:
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    sub $24, %rsp
    movq $0x55112233,(%rsp)
    movabs $5427681975911580161, %rax
    movabs $5427681975911580162, %rbx
    movabs $5427681975911580163, %rcx
    movabs $5427681975911580164, %rdx
    movabs $5427681975911580165, %rbp
    movabs $5427681975911580166, %rdi
    movabs $5427681975911580167, %rsi
    movabs $5427681975911580168, %r8
    movabs $5427681975911580169, %r9
    movabs $5427681975911580170, %r10
    movabs $5427681975911580171, %r11
    movabs $5427681975911580172, %r12
    movabs $5427681975911580173, %r13
    movabs $5427681975911580174, %r14
    movabs $5427681975911580175, %r15
    std
.Lloop_a:
    incq demo_a(%rip)
    cmpq .Lcounter_expected_rax(%rip), %rax
    jne .Lbad_a
    cmpq .Lcounter_expected_rbx(%rip), %rbx
    jne .Lbad_a
    cmpq .Lcounter_expected_rcx(%rip), %rcx
    jne .Lbad_a
    cmpq .Lcounter_expected_rdx(%rip), %rdx
    jne .Lbad_a
    cmpq .Lcounter_expected_rbp(%rip), %rbp
    jne .Lbad_a
    cmpq .Lcounter_expected_rdi(%rip), %rdi
    jne .Lbad_a
    cmpq .Lcounter_expected_rsi(%rip), %rsi
    jne .Lbad_a
    cmpq .Lcounter_expected_r8(%rip), %r8
    jne .Lbad_a
    cmpq .Lcounter_expected_r9(%rip), %r9
    jne .Lbad_a
    cmpq .Lcounter_expected_r10(%rip), %r10
    jne .Lbad_a
    cmpq .Lcounter_expected_r11(%rip), %r11
    jne .Lbad_a
    cmpq .Lcounter_expected_r12(%rip), %r12
    jne .Lbad_a
    cmpq .Lcounter_expected_r13(%rip), %r13
    jne .Lbad_a
    cmpq .Lcounter_expected_r14(%rip), %r14
    jne .Lbad_a
    cmpq .Lcounter_expected_r15(%rip), %r15
    jne .Lbad_a
    cmpq $0x55112233,(%rsp)
    jne .Lbad_a
    pushfq
    testq $0x400,(%rsp)
    lea 8(%rsp),%rsp
    jz .Lbad_a
    cmpq $0,demo_stop(%rip)
    je .Lloop_a
    cld
    add $24,%rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret
.Lbad_a:
    cld
    incq register_errors(%rip)
    call cpu_halt
.size counter_task_a, .-counter_task_a
.global counter_task_b
.type counter_task_b, @function
counter_task_b:
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    sub $24, %rsp
    movq $0x55112233,(%rsp)
    movabs $5427681975911580161, %rax
    movabs $5427681975911580162, %rbx
    movabs $5427681975911580163, %rcx
    movabs $5427681975911580164, %rdx
    movabs $5427681975911580165, %rbp
    movabs $5427681975911580166, %rdi
    movabs $5427681975911580167, %rsi
    movabs $5427681975911580168, %r8
    movabs $5427681975911580169, %r9
    movabs $5427681975911580170, %r10
    movabs $5427681975911580171, %r11
    movabs $5427681975911580172, %r12
    movabs $5427681975911580173, %r13
    movabs $5427681975911580174, %r14
    movabs $5427681975911580175, %r15
    std
.Lloop_b:
    incq demo_b(%rip)
    cmpq .Lcounter_expected_rax(%rip), %rax
    jne .Lbad_b
    cmpq .Lcounter_expected_rbx(%rip), %rbx
    jne .Lbad_b
    cmpq .Lcounter_expected_rcx(%rip), %rcx
    jne .Lbad_b
    cmpq .Lcounter_expected_rdx(%rip), %rdx
    jne .Lbad_b
    cmpq .Lcounter_expected_rbp(%rip), %rbp
    jne .Lbad_b
    cmpq .Lcounter_expected_rdi(%rip), %rdi
    jne .Lbad_b
    cmpq .Lcounter_expected_rsi(%rip), %rsi
    jne .Lbad_b
    cmpq .Lcounter_expected_r8(%rip), %r8
    jne .Lbad_b
    cmpq .Lcounter_expected_r9(%rip), %r9
    jne .Lbad_b
    cmpq .Lcounter_expected_r10(%rip), %r10
    jne .Lbad_b
    cmpq .Lcounter_expected_r11(%rip), %r11
    jne .Lbad_b
    cmpq .Lcounter_expected_r12(%rip), %r12
    jne .Lbad_b
    cmpq .Lcounter_expected_r13(%rip), %r13
    jne .Lbad_b
    cmpq .Lcounter_expected_r14(%rip), %r14
    jne .Lbad_b
    cmpq .Lcounter_expected_r15(%rip), %r15
    jne .Lbad_b
    cmpq $0x55112233,(%rsp)
    jne .Lbad_b
    pushfq
    testq $0x400,(%rsp)
    lea 8(%rsp),%rsp
    jz .Lbad_b
    cmpq $0,demo_stop(%rip)
    je .Lloop_b
    cld
    add $24,%rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret
.Lbad_b:
    cld
    incq register_errors(%rip)
    call cpu_halt
.size counter_task_b, .-counter_task_b
.section .note.GNU-stack,"",@progbits

.section .rodata
.balign 8
.Lcounter_expected_rax: .quad 5427681975911580161
.Lcounter_expected_rbx: .quad 5427681975911580162
.Lcounter_expected_rcx: .quad 5427681975911580163
.Lcounter_expected_rdx: .quad 5427681975911580164
.Lcounter_expected_rbp: .quad 5427681975911580165
.Lcounter_expected_rdi: .quad 5427681975911580166
.Lcounter_expected_rsi: .quad 5427681975911580167
.Lcounter_expected_r8: .quad 5427681975911580168
.Lcounter_expected_r9: .quad 5427681975911580169
.Lcounter_expected_r10: .quad 5427681975911580170
.Lcounter_expected_r11: .quad 5427681975911580171
.Lcounter_expected_r12: .quad 5427681975911580172
.Lcounter_expected_r13: .quad 5427681975911580173
.Lcounter_expected_r14: .quad 5427681975911580174
.Lcounter_expected_r15: .quad 5427681975911580175
