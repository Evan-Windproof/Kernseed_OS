/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
#include <kernseed/api.h>
static int meminfo(int argc,char **argv){
    (void)argc;(void)argv;struct memory_stats snapshot;api_memory(&snapshot);
    terminal_write("pages: usable=");terminal_dec(snapshot.usable);terminal_write(" free=");terminal_dec(snapshot.free);
    terminal_write(" allocated=");terminal_dec(snapshot.allocated);terminal_write(" tables=");terminal_dec(snapshot.tables);
    terminal_write(" mapped=");terminal_dec(snapshot.mapped);terminal_write("\n");return 0;
}
REGISTER_COMMAND(meminfo_command,"meminfo","Read physical memory statistics","meminfo",0,0,meminfo);
