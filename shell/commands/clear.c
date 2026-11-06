/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
static int clear(int argc,char **argv){(void)argc;(void)argv;terminal_clear();return 0;}
REGISTER_COMMAND(clear_command,"clear","Clear the terminal","clear",0,0,clear);
