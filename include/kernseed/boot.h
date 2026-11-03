/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_BOOT_H
#define KERNSEED_BOOT_H
#include <stdint.h>
/* Observable state for diagnostics; these are measured values, not status text. */
struct boot_diagnostics {
    uint64_t stage, cr0, cr3, cr4, efer, rflags;
    uint64_t framebuffer_address, width, height, pitch, bpp;
    uint32_t runtime_result, serial_ok, bss_ok;
};
extern volatile struct boot_diagnostics boot_state;
_Noreturn void kernel_idle(void);
#endif
