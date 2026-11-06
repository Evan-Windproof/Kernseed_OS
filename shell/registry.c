/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
int shell_equal(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
const struct command *command_find(const char *name){
    for(const struct command * const *p=__start_shell_commands;p<__stop_shell_commands;p++)
        if(shell_equal(name,(*p)->name))return *p;
    return 0;
}
int command_registry_valid(void){
    if(__start_shell_commands==&__stop_shell_commands[0])return 0;
    for(const struct command * const *p=__start_shell_commands;p<__stop_shell_commands;p++){
        const struct command *c=*p;
        if(!c||!c->name||!*c->name||!c->description||!c->usage||!c->handler||c->min_args>c->max_args||c->max_args>=SHELL_ARGV)return 0;
        for(const struct command * const *q=__start_shell_commands;q<p;q++)if(shell_equal((*q)->name,c->name))return 0;
    }
    return 1;
}
