/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/console.h>
void console_write(const char *s) {
    while (*s) { serial_putc(*s); framebuffer_putc(*s++); }
}
void console_hex(uint64_t value) {
    char text[19] = "0x";
    for (unsigned i = 0; i < 16; ++i)
        text[2 + i] = "0123456789abcdef"[(value >> ((15 - i) * 4)) & 15];
    text[18] = 0;
    console_write(text);
}
void console_dec(uint64_t value) {
    char text[21];
    unsigned i = 20;
    text[i] = 0;
    do { text[--i] = (char)('0' + value % 10); value /= 10; } while (value);
    console_write(text + i);
}
