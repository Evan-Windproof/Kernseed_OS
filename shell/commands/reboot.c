/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
#include <kernseed/io.h>
static int reboot(int argc,char **argv){
    (void)argc;(void)argv;terminal_write("Rebooting via i8042\n");
    for(unsigned i=0;i<100000;i++)if(!(inb(0x64)&2)){outb(0x64,0xfe);return 0;}
    terminal_write("error: i8042 reset unavailable\n");return 1;
}
REGISTER_COMMAND(reboot_command,"reboot","Reset the PC via i8042","reboot",0,0,reboot);
