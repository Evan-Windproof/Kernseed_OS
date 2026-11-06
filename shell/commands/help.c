/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
static int help(int argc,char **argv){
    if(argc==2){const struct command *c=command_find(argv[1]);if(!c){terminal_write("error: no such command\n");return 1;}
        terminal_write(c->usage);terminal_write(" - ");terminal_write(c->description);terminal_write("\n");return 0;}
    for(const struct command * const *p=__start_shell_commands;p<__stop_shell_commands;p++){
        terminal_write((*p)->name);terminal_write(" - ");terminal_write((*p)->description);terminal_write("\n");}
    return 0;
}
REGISTER_COMMAND(help_command,"help","List registered commands","help [command]",0,1,help);
