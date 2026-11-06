/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
#include <kernseed/memory.h>
static int meminfo(int argc,char **argv){
    (void)argc;(void)argv;uint64_t flags;__asm__ volatile("pushfq; popq %0; cli":"=r"(flags)::"memory");
    struct memory_stats snapshot=memory_state;
    __asm__ volatile("pushq %0; popfq"::"r"(flags):"memory","cc");
    terminal_write("pages: usable=");terminal_dec(snapshot.usable);terminal_write(" free=");terminal_dec(snapshot.free);
    terminal_write(" allocated=");terminal_dec(snapshot.allocated);terminal_write(" tables=");terminal_dec(snapshot.tables);
    terminal_write(" mapped=");terminal_dec(snapshot.mapped);terminal_write("\n");return 0;
}
REGISTER_COMMAND(meminfo_command,"meminfo","Read physical memory statistics","meminfo",0,0,meminfo);
