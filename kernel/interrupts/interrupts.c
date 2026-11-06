/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/interrupts.h>
#include <kernseed/console.h>
#include <kernseed/io.h>
#include <kernseed/runtime.h>

struct idt_gate idt[256] __attribute__((aligned(16)));
volatile struct interrupt_diagnostics irq_state;
volatile struct interrupt_frame last_exception;
extern void (*isr_table[256])(void);
extern void fault_de(void), fault_ud(void), fault_gp(void), fault_pf(void);

static void io_wait(void) { outb(0x80, 0); }
static void pic_write(uint16_t port, uint8_t value) { outb(port, value); io_wait(); }
static void pic_init(void) {
    /* PC/AT 8259 pair, vectors 32..47, 8086 mode. Only PIT IRQ0 allowed.
     * Slave and cascade remain masked; keyboard/RTC/NIC cannot interrupt. */
    pic_write(0x21, 0xff); pic_write(0xa1, 0xff);
    pic_write(0x20, 0x11); pic_write(0xa0, 0x11);
    pic_write(0x21, 32); pic_write(0xa1, 40);
    pic_write(0x21, 4); pic_write(0xa1, 2);
    pic_write(0x21, 1); pic_write(0xa1, 1);
    pic_write(0x21, 0xff); pic_write(0xa1, 0xff);
}
void interrupts_init(void) {
    __asm__ volatile ("cli" ::: "memory");
    gdt_init();
    for (unsigned i = 0; i < 256; ++i) {
        uint64_t address = (uintptr_t)isr_table[i];
        idt[i] = (struct idt_gate){
            .offset_low = address, .selector = 8, .ist = i == 8 ? 1 : 0,
            .attributes = 0x8e, .offset_mid = address >> 16,
            .offset_high = address >> 32, .reserved = 0
        };
    }
    struct table_pointer pointer = {sizeof(idt) - 1, (uintptr_t)idt};
    __asm__ volatile ("lidt %0" : : "m"(pointer) : "memory");
    __asm__ volatile ("sidt %0" : "=m"(irq_state.idtr));
    pic_init();
    irq_state.initialized = 1;
}
void timer_init(void) {
    /* 1,193,182 Hz oscillator / 11,932 = ~99.9985 Hz. Channel 0,
     * low/high divisor bytes, mode 2 rate generator, binary counting. */
    const uint16_t divisor = 11932;
    outb(0x43, 0x34);
    outb(0x40, divisor & 255); outb(0x40, divisor >> 8);
    irq_state.pit_divisor = divisor;
    outb(0x21, 0xfe);
    irq_state.pic_master_mask = inb(0x21);
    irq_state.pic_slave_mask = inb(0xa1);
    /* Do NOT STI here. kernel_idle enables IRQs after BOOT_READY. */
}
static int spurious_irq(unsigned vector) {
    if (vector == 39) {
        outb(0x20, 0x0b);
        return !(inb(0x20) & 0x80); /* No EOI for spurious master IRQ7. */
    }
    if (vector == 47) {
        outb(0xa0, 0x0b);
        if (!(inb(0xa0) & 0x80)) {
            outb(0x20, 0x20); /* Cascade was real; slave IRQ15 was not. */
            return 1;
        }
    }
    return 0;
}
static _Noreturn void exception_halt(struct interrupt_frame *frame) {
    /* No allocation, locks, scheduler, or IRQ-dependent output here.
     * DF has a separate IST stack. Nested diagnostics use serial only. */
    if (irq_state.fatal) {
        irq_state.nested++;
        const char *s = "NESTED EXCEPTION: halted\n";
        while (*s) serial_putc(*s++);
        cpu_halt();
    }
    irq_state.fatal = 1;
    last_exception = *frame;
    irq_state.exception_stack = (uintptr_t)frame;
    if (frame->vector == 14)
        __asm__ volatile ("mov %%cr2, %0" : "=r"(irq_state.exception_cr2));
    console_write("EXCEPTION vector="); console_dec(frame->vector);
    console_write(" error="); console_hex(frame->error);
    console_write("\nRIP="); console_hex(frame->rip);
    console_write(" RSP="); console_hex(frame->rsp);
    console_write("\nCS="); console_hex(frame->cs);
    console_write(" RFLAGS="); console_hex(frame->rflags);
    if (frame->vector == 14) {
        console_write(frame->error & 1 ? "\nPF: protection violation" : "\nPF: not present");
        console_write(frame->error & 2 ? " write" : " read");
        console_write(frame->error & 16 ? " instruction" : " data");
        console_write("\nCR2="); console_hex(irq_state.exception_cr2);
    }
    console_write("\nEXCEPTION_HALTED\n");
    cpu_halt();
}
void interrupt_dispatch(struct interrupt_frame *frame) {
    uintptr_t stack;
    uint64_t flags;
    __asm__ volatile ("mov %%rsp, %0; pushfq; popq %1" : "=r"(stack), "=r"(flags));
    irq_state.dispatch_rsp = stack;
    /* The compiler prologue may move RSP; exact ABI is verified by GDB at
     * the first instruction. IF/DF must nevertheless be clear throughout C. */
    if (flags & 0x600) irq_state.entry_if_errors++;
    if (frame->vector < 32) exception_halt(frame);
    if (frame->vector >= 32 && frame->vector < 48) {
        if (spurious_irq((unsigned)frame->vector)) { irq_state.spurious++; return; }
        if (frame->vector == 32) irq_state.ticks++;
        else irq_state.other_irq++;
        if (frame->vector >= 40) outb(0xa0, 0x20);
        outb(0x20, 0x20);
        if (frame->vector == 32) irq_state.timer_eoi++;
        return;
    }
    /* CPL0 software interrupt demonstration, NOT a syscall ABI. */
    if (frame->vector == 129) return; /* Scheduler trap, handled after dispatch. */
    if (frame->vector == 128) { irq_state.software_count++; return; }
    exception_halt(frame); /* Unexpected vector: fail closed, never guess a return. */
}
void interrupt_fault_test(void) {
#if KERNSEED_TEST_FAULT == 1
    fault_de();
#elif KERNSEED_TEST_FAULT == 2
    fault_ud();
#elif KERNSEED_TEST_FAULT == 3
    fault_gp();
#elif KERNSEED_TEST_FAULT == 4
    fault_pf();
#elif KERNSEED_TEST_FAULT == 5
    /* #GP delivery fails with #NP (not-present #GP gate). Contributory
     * exception during contributory delivery => actual #DF, not INT 8. */
    idt[13].attributes &= (uint8_t)~0x80;
    __asm__ volatile ("" ::: "memory");
    fault_gp();
#endif
}
