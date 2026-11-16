/* SPDX-License-Identifier: GPL-3.0-or-later */
void _start(void){*(volatile unsigned long *)0xffffffff80000000ULL=1;for(;;);}
