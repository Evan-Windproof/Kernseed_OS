/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
#include <kernseed/api.h>
static int uptime(int argc,char **argv){
    (void)argc;(void)argv;uint64_t ticks=api_ticks(),divisor=irq_state.pit_divisor;
    uint64_t seconds=(ticks/1193182)*divisor+((ticks%1193182)*divisor)/1193182;
    terminal_write("uptime: ticks=");terminal_dec(ticks);terminal_write(" seconds=");terminal_dec(seconds);terminal_write("\n");return 0;
}
REGISTER_COMMAND(uptime_command,"uptime","Read elapsed PIT time","uptime",0,0,uptime);
