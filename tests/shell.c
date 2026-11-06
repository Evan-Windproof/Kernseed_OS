/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
#include <stdio.h>
#include <string.h>
static char output[4096];
void terminal_write(const char *s){if(strlen(output)+strlen(s)>=sizeof(output))__builtin_trap();strcat(output,s);}
static int run(const char *s){char line[128];strcpy(line,s);output[0]=0;return shell_execute(line);}
static int probe(int argc,char **argv){return argc==2&&shell_equal(argv[1],"test")?0:1;}
REGISTER_COMMAND(test_extension,"probe","Independent test extension","probe test",1,1,probe);
int main(void){
    char *argv[SHELL_ARGV];
    char empty[]=" \t ";if(shell_parse(empty,argv,8)!=0)return 1;
    char words[]="echo \"a b\" '' c\\ d";
    if(shell_parse(words,argv,8)!=4||strcmp(argv[1],"a b")||strcmp(argv[2],"")||strcmp(argv[3],"c d"))return 2;
    char bad[]="echo 'open";if(shell_parse(bad,argv,8)!=-2)return 3;
    char escape[]="echo trailing\\";if(shell_parse(escape,argv,8)!=-2)return 4;
    char many[]="echo a b c d e f g h";if(shell_parse(many,argv,8)!=-1)return 5;
    if(!command_registry_valid()||!command_find("echo")||!command_find("probe"))return 6;
    if(run("help")||!strstr(output,"probe - Independent test extension")||!strstr(output,"echo - Echo parsed arguments"))return 7;
    if(run("probe test")||run("echo \"a b\" c")||strcmp(output,"a b c\n"))return 8;
    if(run("probe")!=2||!strstr(output,"usage: probe test"))return 9;
    if(run("missing")!=127||run("echo a b c d e f g h")!=2)return 10;
    puts("PASS parser/registry: quotes, escapes, empty args, bounds, independent registration, automatic help and dispatch");return 0;
}
