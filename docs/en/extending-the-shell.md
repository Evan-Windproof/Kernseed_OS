# Add a command

```c
/* Tutorial only: shell/commands/hello.c */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>
static int hello(int argc, char **argv) {
    (void)argc;
    terminal_write("Hello, ");
    terminal_write(argv[1]);
    terminal_write("!\n");
    return 0;
}
REGISTER_COMMAND(hello_command,"hello","Greet a name","hello name",1,1,hello);
```
Create this file in a learning checkout, then make iso and make run. Makefile discovers shell/commands/*.c automatically; parser/dispatch/help need no edits. Test help, help hello, hello "Ada Lovelace", and missing/extra arguments. argc includes the command; registry min/max exclude it. argv is bounded by argc, not NULL-terminated, and valid only during the call. Metadata must have static lifetime. This tutorial command is not installed by the documentation change.

[Registry source](../../shell/registry.c) · [Terminal](terminal-shell.md)

[Index](index.md)
