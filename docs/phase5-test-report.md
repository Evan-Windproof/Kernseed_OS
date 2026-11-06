# Phase 5 实施与测试报告

日期：2026-11-06（Asia/Taipei）。基线：main `8e395fc`（已合入Phase 4），分支codex/phase5。
实际环境：Debian 13云环境、GCC14.2、QEMU TCG/SeaBIOS单核、GDB16.3，未在本次WSL/实机验收。
编码前完整基线回归通过，调度使用显式5秒快速复核，不冒充Phase 4的120秒耐久记录。

## 实现

独立PS/2键盘驱动与IRQ1字符环，终端滚屏/退格/清屏，独立Shell任务和行编辑，
无宿主服务的freestanding解析器，编译期Command Registry，独立命令模块与自动help。
八条命令为help、clear、about、meminfo、tasks、uptime、reboot、echo。
注册表支持名称、描述、用法、参数范围和统一handler；不存在命令名称的大型if-else链。
扩展只需增加命令文件，教程完整示例见 [shell-development.md](shell-development.md)。

默认PIC放行IRQ0/IRQ1（mask=fc），其余保持屏蔽；键盘IRQ不输出、不调度、不分配。
Shell用既有调度器创建为第6普通槽，无输入sleep(1)，与真实A/B/C抢占任务共存。
meminfo/tasks读取真实快照，uptime读取实际PIT tick，重启操作为真实i8042硬件复位。
没有引入HoofTTY功能、MLP内容、动态插件或用户态依赖。

## 实际执行结果

- `make test-host`：参数空白、单双引号、转义、空参数、数量边界、未闭合输入、未知命令通过。
- 独立probe注册于宿主测试，不修改核心分发/help；help自动出现probe与echo，原有处理接口继续工作。
- framebuffer宿主测试：字形、RGB顺序、pitch/边界哨兵、退格擦除、真实滚屏内容、清屏、非法格式拒绝通过。
- `make test`：Phase 1–4全部既有检查通过（旧fixture明确SCHEDULER/CONSOLE_SHELL模式），最终恢复默认Shell镜像。
- 128/256 MiB各自真实QEMU冷启动，QMP逐字符发送实际PS/2键盘事件，八条命令均执行并得到预期结果。
- 参数/输入：引号保留空格、空参数、Shift大小写、Backspace修正、Tab/空行、未知命令、错误用法、
  未知help目标、过多参数、未闭合引号、140字节超长行及下一条命令恢复，全部通过。
- meminfo账目一致且读取实际16个任务数据页/映射；tasks包含真实shell RUNNING与其他TCB；uptime tick增长。
- GDB检查实际IRQ1字节/中断、无意外输入丢失/硬件错误、PIC masks、调度抢占与零寄存器错误，内存账目一致。
- 背压：通过Shell真实sleep请求延长其阻塞，实际PS/2键盘填满255字符环；GDB确认dropped>0，
  唤醒Shell后显示溢出错误，Enter丢弃，echo fifo-recovered继续执行，未执行截断的旧内容。
- i8042鼠标端口关闭；注入鼠标移动事件后键盘仍可继续执行命令，防止共享控制器阻塞。
- QMP截取实际屏幕，验证清屏背景与提示符/echo字形像素；截图也经实际查看。
- 输入reboot后未调用QMP reset命令，观察到新的启动/Shell提示符，再输入about成功，证明真实硬件复位后交互仍可工作。
- `make reproducible`的结果及最终SHA-256记录于证据目录；同目录/工具链的保证不扩大到任意版本或路径。

详细命令、串口全文、GDB脚本、截图与结果见 [evidence/phase-5](evidence/phase-5/)。
旧调度压力复核本阶段使用5秒快速配置；不把该值描述成120秒或长期压力。

## 调试记录

首轮QEMU已输出完整自动help，但测试按文件字节大小切分经Python CRLF转换后的文本，
误认为帮助第一项缺失。修正为一致的文本字符偏移；保留原失败日志，未更改命令或删除帮助断言。
之后命令、参数、键盘、重启通过。补充真实FIFO背压验证及终端像素/滚屏检查。
检查设计时明确禁用未使用AUX端口，避免GUI鼠标填满共享i8042控制器；最终回归包含对应输入事件。
没有已知阻止上述验收的内核错误。

## 代码边界与限制

主要新增keyboard.h/terminal.h/shell.h、键盘/终端后端、shell/parser.c、registry.c、dispatch.c、shell.c、
八个独立commands文件及tests/shell.c、tests/phase5.py。链接脚本增加KEEP的命令指针区；
Makefile自动发现命令文件，保持iso/run/headless/debug/test接口。原许可证与第三方声明不变。

当前为US ASCII、i8042 Set 1、单核、单终端所有者、有限字体；无USB/Unicode/历史/管道/脚本/完整VT。
字体保留教学大写渲染，实际输入与命令名仍区分大小写，串口原样显示。
headless的UART仅输出，交互用QEMU窗口或QMP真实键盘；不宣称串口键入可操作Shell。
输入行127字节、总token8个，超限明确报错；满环可能需要再按Enter丢弃，不执行部分命令。

普通配置新增Shell栈，CONSOLE_SHELL=0仅用于旧阶段精确状态/容量fixture。
默认demo_stop只让旧演示任务退出，不在Shell存在时执行Phase 4专用满额/回收fixture；Shell继续运行。
没有Shell退出/登录/用户进程隔离。HoofTTY仅有模块边界基础，无专属功能实现。
提交标题Add Kernseed OS Phase 5；推送/PR结果见交付消息，不自动合并main。
