/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
#include <kernseed/api.h>
static int list_tasks(int argc,char **argv){
    (void)argc;(void)argv;struct task snapshot[TASK_LIMIT];api_tasks(snapshot);
    static const char *names[]={"UNUSED","RUNNING","READY","BLOCKED","TERMINATED"};
    terminal_write("id name state switches runtime_ticks\n");
    for(unsigned i=0;i<TASK_LIMIT;i++){
        struct task *t=&snapshot[i];if(t->status==UNUSED)continue;
        terminal_dec(t->id);terminal_write(" ");terminal_write(t->name);terminal_write(" ");
        terminal_write(t->status<=TERMINATED?names[t->status]:"INVALID");terminal_write(" ");terminal_dec(t->switches);
        terminal_write(" ");terminal_dec(t->runtime_ticks);terminal_write("\n");
    }return 0;
}
REGISTER_COMMAND(tasks_command,"tasks","Read kernel task states","tasks",0,0,list_tasks);
