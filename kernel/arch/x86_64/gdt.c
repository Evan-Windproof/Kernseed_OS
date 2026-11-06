/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/interrupts.h>

uint64_t gdt[5+2*KERNSEED_USERMODE] __attribute__((aligned(16)));
struct tss64 kernel_tss;
unsigned char df_stack[16384] __attribute__((aligned(16)));
extern unsigned char boot_stack_top[];
extern void gdt_load(const struct table_pointer *pointer);

void gdt_init(void) {
    /* CPL0 code: present, executable, L=1 D=0. Data: present, L=0.
     * TSS descriptor occupies TWO slots; optional user code/data follow it. */
    gdt[0] = 0;
    gdt[1] = 0x00af9a000000ffff;
    gdt[2] = 0x00cf92000000ffff;
    kernel_tss.rsp[0] = (uintptr_t)boot_stack_top;
    kernel_tss.ist[0] = (uintptr_t)(df_stack + sizeof(df_stack));
    kernel_tss.iomap = sizeof(kernel_tss); /* No I/O permission bitmap. */
    uint64_t base = (uintptr_t)&kernel_tss, limit = sizeof(kernel_tss) - 1;
    gdt[3] = (limit & 0xffff) | ((base & 0xffffff) << 16)
           | (0x89ULL << 40) | ((limit & 0xf0000) << 32) | ((base & 0xff000000) << 32);
    gdt[4] = base >> 32;
    if(KERNSEED_USERMODE){gdt[5]=0x00affa000000ffff;gdt[6]=0x00cff2000000ffff;}
    struct table_pointer pointer = {sizeof(gdt) - 1, (uintptr_t)gdt};
    gdt_load(&pointer);
    __asm__ volatile ("sgdt %0" : "=m"(irq_state.gdtr));
    __asm__ volatile ("str %0" : "=m"(irq_state.tr));
}
