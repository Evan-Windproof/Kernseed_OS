# GDB 调试指南

在两个终端中均切到仓库根目录：

```sh
# 终端一
make debug
# 终端二
gdb -x scripts/debug.gdb
```

脚本加载带 DWARF 符号的 `build/kernseed.elf`，连接 `127.0.0.1:1234`，
设置 `hbreak *kernel_main` 并继续。星号让断点落在第一条机器指令，而不是 C 行号对应的
函数序言之后；这是验证入口栈对齐的前提。硬件断点避免在尚未映射的内核地址写入软件断点。

```gdb
info registers rip rsp eflags cr0 cr3 cr4
p/x (unsigned long)$rsp & 15
x/8gx $rsp
delete breakpoints
hbreak kernel_idle
continue
p/x boot_state
x/8i kernel_idle
```

C 入口 RSP 模 16 应为 8，DF/IF 均为 0。idle 前 `boot_state.stage` 应为 2，
`runtime_result` 为 0，`bss_ok` 和 `serial_ok` 为 1。
`cr0` 含 PG/PE，`cr4` 含 PAE，`efer` 含 LME/LMA，`cr3` 是实际启动页表物理基址。
`boot_state` 字段由硬件读取或执行测试产生，不是手填状态值。

`kernel_idle_halt` 是 HLT 指令位置；在 HLT 单步可能一直等中断，宜在执行前断住，
或 Ctrl-C 暂停后检查。当前内核没有定时器，不应等待 tick 变化。

`make test` 使用独立 UNIX GDB socket 自动验证上述条件，保存 `gdb.log` 和执行脚本。
宿主 GDB 本身的动态库、XML/符号解析等错误不应视作内核失败，也不能忽略。
调试端口具备完全控制虚拟机的能力，保持仅监听本机，不对外暴露。
