/* SPDX-License-Identifier: GPL-3.0-or-later */
void _start(void){__asm__ volatile("xor %%rsp,%%rsp; mov $1,%%eax; int $0x82":::"rax","memory");for(;;);}
