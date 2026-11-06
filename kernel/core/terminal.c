/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/terminal.h>
#include <kernseed/console.h>
/* One shell task owns interactive display. Never called by keyboard IRQ. */
void terminal_write(const char *s){console_write(s);}
void terminal_clear(void){
    framebuffer_clear();
    const char *s="\033[2J\033[H";while(*s)serial_putc(*s++);
}
void terminal_erase(void){
    framebuffer_backspace();
    serial_putc('\b');serial_putc(' ');serial_putc('\b');
}

void terminal_dec(uint64_t value){
    char text[21];unsigned at=20;text[at]=0;
    do{text[--at]=(char)('0'+value%10);value/=10;}while(value);
    terminal_write(text+at);
}
