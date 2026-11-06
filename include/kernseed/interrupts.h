/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_INTERRUPTS_H
#define KERNSEED_INTERRUPTS_H
#include <stdint.h>
#include <stddef.h>

/* Matches isr-common.S exactly. Long mode pushes SS/RSP even at CPL0.
 * vector/error are normalized by the generated per-vector stubs. */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
    uint64_t vector, error, rip, cs, rflags, rsp, ss;
};
_Static_assert(sizeof(struct interrupt_frame) == 176, "ISR frame size");
_Static_assert(offsetof(struct interrupt_frame, vector) == 120, "ISR vector offset");
_Static_assert(offsetof(struct interrupt_frame, rsp) == 160, "ISR RSP offset");

struct table_pointer { uint16_t limit; uint64_t base; } __attribute__((packed));
struct idt_gate {
    uint16_t offset_low, selector;
    uint8_t ist, attributes;
    uint16_t offset_mid;
    uint32_t offset_high, reserved;
} __attribute__((packed));
struct tss64 {
    uint32_t reserved0;
    uint64_t rsp[3], reserved1, ist[7], reserved2;
    uint16_t reserved3, iomap;
} __attribute__((packed));
_Static_assert(sizeof(struct idt_gate) == 16, "64-bit IDT gate");
_Static_assert(sizeof(struct tss64) == 104, "64-bit TSS");

struct interrupt_diagnostics {
    uint64_t initialized, ticks, timer_eoi, software_count, other_irq, spurious;
    uint64_t entry_if_errors, register_probe_result;
    uint64_t pit_divisor, pic_master_mask, pic_slave_mask;
    uint64_t dispatch_rsp, exception_stack, exception_cr2, fatal, nested;
    struct table_pointer gdtr, idtr;
    uint16_t tr;
};
extern volatile struct interrupt_diagnostics irq_state;
extern volatile struct interrupt_frame last_exception;
extern struct idt_gate idt[256];
extern uint64_t gdt[5+2*KERNSEED_USERMODE];
extern struct tss64 kernel_tss;
extern unsigned char df_stack[16384];
void gdt_init(void);
void interrupts_init(void);
void timer_init(void);
void interrupt_dispatch(struct interrupt_frame *frame);
int interrupt_register_probe(void);
void interrupt_fault_test(void);
_Noreturn void cpu_halt(void);
#endif
