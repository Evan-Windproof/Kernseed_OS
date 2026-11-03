/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <limine.h>
#include <kernseed/boot.h>
#include <kernseed/console.h>
#include <kernseed/runtime.h>

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3)
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0
};
__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER

volatile struct boot_diagnostics boot_state;
static volatile unsigned char bss_probe[256];

static _Noreturn void panic(const char *reason) {
    boot_state.stage = 0xff;
    console_write("PANIC: "); console_write(reason); console_write("\n");
    kernel_idle();
}
_Noreturn void kernel_main(void) {
    boot_state.serial_ok = serial_init();
    console_write("Kernseed OS - Phase 1\n");
    console_write("Independent x86-64 monolithic kernel\n");
    if (!LIMINE_BASE_REVISION_SUPPORTED) panic("Limine base revision 3 unsupported");
    boot_state.stage = 1;
    boot_state.bss_ok = 1;
    for (unsigned i = 0; i < sizeof(bss_probe); ++i)
        if (bss_probe[i]) boot_state.bss_ok = 0;
    if (!boot_state.bss_ok) panic("BSS not zero initialized");
    boot_state.runtime_result = (uint32_t)runtime_selftest();
    if (boot_state.runtime_result) panic("C runtime selftest failed");

    struct limine_framebuffer_response *response = framebuffer_request.response;
    if (!response || !response->framebuffer_count || !response->framebuffers)
        panic("No framebuffer response");
    struct limine_framebuffer *fb = response->framebuffers[0];
    if (!framebuffer_init(fb)) panic("Unsupported framebuffer format");
    boot_state.framebuffer_address = (uintptr_t)fb->address;
    boot_state.width = fb->width; boot_state.height = fb->height;
    boot_state.pitch = fb->pitch; boot_state.bpp = fb->bpp;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(boot_state.cr0));
    __asm__ volatile ("mov %%cr3, %0" : "=r"(boot_state.cr3));
    __asm__ volatile ("mov %%cr4, %0" : "=r"(boot_state.cr4));
    __asm__ volatile ("pushfq; popq %0" : "=r"(boot_state.rflags));
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(0xc0000080));
    boot_state.efer = ((uint64_t)high << 32) | low;
    if (!(boot_state.cr0 & (1ULL << 31)) || !(boot_state.cr4 & (1ULL << 5)) ||
        !(boot_state.efer & (1ULL << 10)) || (boot_state.rflags & (1ULL << 9)))
        panic("Unexpected CPU boot state");

    /* Repeat the title after framebuffer initialization, so both sinks show it. */
    console_write("Kernseed OS - Phase 1\n");
    console_write("Boot: Limine base revision 3\n");
    console_write("BSS: zero checked\nC runtime: selftest passed\n");
    console_write("CPU: long mode, paging, IRQs disabled\n");
    console_write("CR3: "); console_hex(boot_state.cr3); console_write("\n");
    console_write("Framebuffer: "); console_dec(fb->width); console_write("x");
    console_dec(fb->height); console_write("x"); console_dec(fb->bpp); console_write("\n");
    console_write("No scheduler, keyboard or shell yet\n");
    console_write("BOOT_READY: entering idle loop\n");
    boot_state.stage = 2;
    kernel_idle();
}
