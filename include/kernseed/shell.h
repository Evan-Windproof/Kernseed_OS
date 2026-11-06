/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_SHELL_H
#define KERNSEED_SHELL_H
#include <stddef.h>
#include <stdint.h>
#define SHELL_LINE 128
#define SHELL_ARGV 8
struct command {
    const char *name,*description,*usage;
    unsigned min_args,max_args;
    int (*handler)(int argc,char **argv);
};
#define REGISTER_COMMAND(symbol,n,d,u,min,max,fn) \
    static const struct command symbol={n,d,u,min,max,fn}; \
    static const struct command * const symbol##_registration \
    __attribute__((used,section("shell_commands"),aligned(8)))=&symbol
extern const struct command * const __start_shell_commands[], * const __stop_shell_commands[];
int shell_parse(char *line,char **argv,size_t capacity);
int shell_execute(char *line);
int shell_equal(const char *a,const char *b);
const struct command *command_find(const char *name);
int command_registry_valid(void);
void shell_task(void);
struct shell_diagnostics {uint64_t ready,commands,errors,overlong,last_status;};
extern volatile struct shell_diagnostics shell_state;
#endif
