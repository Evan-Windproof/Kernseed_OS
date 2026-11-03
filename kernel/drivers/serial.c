/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/console.h>
#include <kernseed/io.h>

#define COM1 0x3f8
static bool available;

bool serial_init(void) {
    /* PC 16550-compatible UART, polling, 115200 baud, 8N1; no IRQs. */
    outb(COM1 + 1, 0);
    outb(COM1 + 3, 0x80);
    outb(COM1, 1);
    outb(COM1 + 1, 0);
    outb(COM1 + 3, 3);
    outb(COM1 + 2, 0xc7);
    outb(COM1 + 4, 0x1e); /* Loopback proves the UART responds. */
    outb(COM1, 0xae);
    available = inb(COM1) == 0xae;
    outb(COM1 + 4, 0x0b);
    return available;
}
void serial_putc(char c) {
    if (!available) return;
    if (c == '\n') serial_putc('\r');
    /* A missing/broken UART must not hang the framebuffer console. */
    for (unsigned i = 0; i < 100000; ++i) {
        if (inb(COM1 + 5) & 0x20) { outb(COM1, (uint8_t)c); return; }
        __asm__ volatile ("pause");
    }
}
