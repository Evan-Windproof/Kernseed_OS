/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
#include <kernseed/keyboard.h>
#include <kernseed/scheduler.h>
volatile struct shell_diagnostics shell_state;
void shell_task(void){
    char line[SHELL_LINE];unsigned used=0;bool overflow=false;
    if(!command_registry_valid()){terminal_write("error: invalid command registry\n");return;}
    shell_state.ready=1;terminal_write("\nKernseed Shell: type help\nks> ");
    for(;;){
        int c=keyboard_getchar();if(c<0){task_sleep(1);continue;}
        if(c==127){overflow=true;terminal_write("\nerror: input overflow; press Enter to discard\n");}
        else if(c=='\n'){
            terminal_write("\n");
            if(overflow){terminal_write("error: line too long; discarded\n");shell_state.overlong++;shell_state.errors++;}
            else {line[used]=0;int status=shell_execute(line);shell_state.last_status=(uint64_t)status;shell_state.commands++;if(status)shell_state.errors++;}
            used=0;overflow=false;terminal_write("ks> ");
        }else if(c==21){while(used){terminal_erase();used--;}overflow=false;}
        else if(c=='\b'){if(!overflow&&used){used--;terminal_erase();}}
        else if(c=='\t'||(c>=32&&c<127)){
            if(overflow)continue;
            if(used==SHELL_LINE-1){overflow=true;continue;}
            line[used++]=(char)c;char text[2]={(char)c,0};terminal_write(c=='\t'?" ":text);
        }
    }
}
