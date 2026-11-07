# 新增Shell命令

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
在学习检出中创建此文件，然后make iso、make run。Makefile自动发现shell/commands/*.c，无需修改parser/dispatch/help。测试help、help hello、hello "Ada Lovelace"及缺少/过多参数。argc含命令名，注册参数范围不含命令名；argv不保证NULL结尾，仅本次调用有效；元数据必须静态存活。本轮文档未安装此教学命令。

[Registry source](../../shell/registry.c) · [Terminal](terminal-shell.md)

[Index](index.md)
