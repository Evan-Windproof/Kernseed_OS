/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_CONSOLE_H
#define KERNSEED_CONSOLE_H
#include <stdbool.h>
#include <stdint.h>
struct limine_framebuffer;
bool serial_init(void);
void serial_putc(char c);
bool framebuffer_init(struct limine_framebuffer *fb);
void framebuffer_putc(char c);
void framebuffer_clear(void);
void framebuffer_backspace(void);
void console_write(const char *s);
void console_hex(uint64_t value);
void console_dec(uint64_t value);
#endif
