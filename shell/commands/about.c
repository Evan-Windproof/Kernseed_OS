/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
static int about(int argc,char **argv){(void)argc;(void)argv;terminal_write("Kernseed OS Phase 7 - x86-64 educational monolithic kernel\nPolyForm Noncommercial 1.0.0; one built-in Ring 3 task; no general ELF loader\n");return 0;}
REGISTER_COMMAND(about_command,"about","Show system information","about",0,0,about);
