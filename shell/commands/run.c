/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/user.h>
#include <kernseed/terminal.h>
static int run(int argc,char **argv){(void)argc;if(user_run(argv[1])){terminal_write("error: unavailable application\n");return 1;}return 0;}
REGISTER_COMMAND(run_command,"run","Run one Ring 3 native application","run name",1,1,run);
