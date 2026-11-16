/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
static int apps(int argc,char **argv){(void)argc;(void)argv;terminal_write("hello counter probe privileged kernelread kernelwrite badflags badstack\n");return 0;}
REGISTER_COMMAND(apps_command,"apps","List embedded native applications","apps",0,0,apps);
