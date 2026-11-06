/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_KEYBOARD_H
#define KERNSEED_KEYBOARD_H
#include <stdbool.h>
#include <stdint.h>
struct keyboard_diagnostics {uint64_t ready,irqs,bytes,dropped,errors;};
extern volatile struct keyboard_diagnostics keyboard_state;
bool keyboard_init(void);
void keyboard_irq(void);
int keyboard_getchar(void);
#endif
