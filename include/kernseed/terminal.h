/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_TERMINAL_H
#define KERNSEED_TERMINAL_H
#include <stdint.h>
void terminal_dec(uint64_t value);
void terminal_write(const char *s);
void terminal_clear(void);
void terminal_erase(void);
#endif
