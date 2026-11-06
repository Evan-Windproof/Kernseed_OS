# 模块化 Shell 开发指南（Phase 5）

## 模块边界

| 模块 | 职责 |
|---|---|
| kernel/drivers/keyboard.c | PC i8042首端口初始化、IRQ1、Set 1解码、有界字符环 |
| kernel/core/terminal.c | Shell显示接口；文字/整数/退格/清屏，私下连接串口与framebuffer |
| kernel/drivers/framebuffer.c | 字形、光标位置、滚屏、擦除；不理解命令 |
| shell/shell.c | 独立内核任务，提示符、行编辑、输入长度与溢出恢复 |
| shell/parser.c | 原地参数解析；不接触硬件或命令名 |
| shell/registry.c | 描述符查找与唯一性/参数范围校验 |
| shell/dispatch.c | 查表、参数校验、调用统一handler；没有命令if-else链 |
| shell/commands/*.c | 每条命令独立实现并声明自己的元数据 |

IRQ仅解码并生产字符，不打印、不解析、不分配内存。Shell任务取字符；无输入时sleep(1)，
不在关闭IRQ时永久HLT。只有Shell拥有交互终端，A/B/C继续真实抢占运行，异常诊断仍可独立输出。
终端数字输出也走terminal接口，命令不依赖具体framebuffer或Linux标准I/O。

## Command Registry

每条命令有name、description、usage、min_args/max_args、`int handler(int argc, char **argv)`。
`argc`包括命令名；元数据的参数范围不包括命令名。handler返回0成功、非0错误。
argv不保证有结尾NULL，必须按argc访问。argv/字符串只在本次调用期间有效，不要保存到下一条命令。

REGISTER_COMMAND声明静态描述符及shell_commands节里的指针。链接脚本KEEP并定义
__start_shell_commands/__stop_shell_commands；registry与help都遍历同一个表。
初始化拒绝空/重复名称、缺失handler和不合法参数范围。描述符为编译期const，不引入动态插件。
Makefile自动发现shell/commands/*.c，不需要修改中央命令清单、解析器、分发器或help实现。

## 新增命令完整示例

创建 `shell/commands/hello.c`：

```c
/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
#include <kernseed/terminal.h>

static int hello(int argc, char **argv) {
    (void)argc;  /* 注册表已保证恰好一个参数。 */
    terminal_write("Hello, ");
    terminal_write(argv[1]);
    terminal_write("!\n");
    return 0;
}
REGISTER_COMMAND(hello_command, "hello", "Greet a name", "hello name", 1, 1, hello);
```

随后：

```sh
make iso
make run
```

在QEMU键盘输入 `help`，列表会自动出现hello；`help hello`显示用法；
`hello "Ada Lovelace"`执行handler；`hello`会由分发器统一报告usage。
上例是扩展教程，不是已加入当前镜像的命令。实际扩展验证是独立文件echo.c，
另有宿主测试的probe命令，均不修改help/parser/dispatch。

命令名建议小写ASCII、无空白且唯一；所有字符串/名称元数据必须保持静态生命周期。
保持handler短小，不从IRQ调用Shell；耗时工作需要明确的任务接口，不把不可恢复异常当错误返回。
未来HoofTTY若更换显示后端，先替换terminal.c；命令只使用terminal_*输出。
本阶段不提供HoofTTY专属功能、MLP内容、动态加载器、脚本系统或用户态接口。

## 输入与参数规则

- 行缓冲128字节：最多127个输入字节；超出后整行丢弃，不执行前缀。
- 最多8个token（命令名+7参数）；超过返回错误。
- 空白分隔，支持单/双引号和反斜杠转义、空字符串参数；不支持管道、重定向、变量或POSIX完整语义。
- 反斜杠在两类引号内外都转义下一字节；未闭合引号或末尾反斜杠报错。
- Enter提交，Backspace删除，Tab作为空白，左Ctrl-U清行；无方向键编辑/历史。
- 输入环256槽（可用255）；满后标记数据丢失，清队列并拒绝该行，等待Enter丢弃恢复，不能执行截断命令。
- 真实字节为US ASCII，命令名区分大小写；现有教学字体将字母显示为大写，串口保留实际大小写。

键盘为QEMU PC/SeaBIOS i8042翻译Set 1；支持左右Shift、Caps Lock、基本US字符。
禁用未使用的AUX端口，避免鼠标占住共享控制器；不支持USB键盘、Unicode、输入法或任意固件。
部分ASCII符号未绘制完整字体，不把此终端称为完整VT/ANSI终端。

## 初始命令与真实数据

help/clear/about/meminfo/tasks/uptime/reboot均在独立文件中。echo是独立扩展示例。
meminfo对PMM/VMM统计做短CLI快照，单位为4 KiB页；tasks快照TCB，不伪造任务状态；
uptime用实际IRQ0 tick与PIT分频计算整数秒，不是宿主墙钟。
reboot通过i8042复位；普通运行脚本允许重启，自动故障测试仍保留-no-reboot以保存错误现场。

交互使用 `make run` 的QEMU窗口键盘。`make headless`显示串口日志，当前不实现UART RX输入；
无窗口自动测试通过QMP向实际PS/2设备注入按键，不把串口写入伪装成键盘。

## 测试与复核

```sh
make test-host        # parser/registry/独立probe与echo/自动help、framebuffer边界与滚屏
make test-shell       # 真实IRQ1、命令/错误输入/显示/背压/重启（128/256 MiB）
make test             # 全部旧回归 + 默认Shell集成
```

旧阶段测试使用CONSOLE_SHELL=0夹具，因为它们会精确验证仅IRQ0或“所有普通任务已退出”的场景；
之后恢复默认CONSOLE_SHELL=1并验证真实键盘和Shell与抢占共存。不得把夹具当作Shell验收。
结果与环境见 [Phase 5报告](phase5-test-report.md)。
