/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
/* Extension demonstration: no parser, dispatcher or help changes. */
static int echo(int argc,char **argv){
    for(int i=1;i<argc;i++){if(i>1)terminal_write(" ");terminal_write(argv[i]);}
    terminal_write("\n");return 0;
}
REGISTER_COMMAND(echo_command,"echo","Echo parsed arguments","echo [text ...]",0,7,echo);
