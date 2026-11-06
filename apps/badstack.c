/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
void _start(void){__asm__ volatile("xor %%rsp,%%rsp; mov $1,%%eax; int $0x82":::"rax","memory");for(;;);}
