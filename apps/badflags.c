/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include "abi.h"
void _start(void){__asm__ volatile("pushfq; orq $0x4000,(%%rsp); popfq":::"memory","cc");call(1,0,0);write_text("FLAGS SANITIZED\n");finish();}
