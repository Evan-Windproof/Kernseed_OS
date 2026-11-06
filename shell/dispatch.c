/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
int shell_execute(char *line){
    char *argv[SHELL_ARGV];int argc=shell_parse(line,argv,SHELL_ARGV);
    if(argc<0){terminal_write(argc==-1?"error: too many arguments\n":"error: malformed quote or escape\n");return 2;}
    if(!argc)return 0;
    const struct command *c=command_find(argv[0]);
    if(!c){terminal_write("error: unknown command: ");terminal_write(argv[0]);terminal_write("\n");return 127;}
    if((unsigned)(argc-1)<c->min_args||(unsigned)(argc-1)>c->max_args){terminal_write("usage: ");terminal_write(c->usage);terminal_write("\n");return 2;}
    return c->handler(argc,argv);
}
