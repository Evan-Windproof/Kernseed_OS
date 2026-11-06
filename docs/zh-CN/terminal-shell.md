# 终端与Shell

IRQ1将i8042翻译Set1按键解码到可用255字符的环形队列，IRQ不输出或解析。shell_task在无输入时休眠，编辑有界行后依次调用parser、registry、dispatcher和handler。terminal_*隔离串口/Framebuffer。help遍历链接器注册表。输入ASCII且区分大小写，有限字体大写显示；UART仅输出。最多127字节、含命令名共8token，支持引号、转义、空参数。溢出整行丢弃，Enter恢复；支持退格和左Ctrl-U，无管道/历史/POSIX展开。

[Source / 源码](../../shell/dispatch.c) · [Index / 目录](index.md)

## 实践与验证

```sh
make test-shell
```

观察真实客户机状态：keyboard_state / shell_state / framebuffer pixels / reboot. 测试需QEMU/GDB位于PATH或配置对应环境变量。故障夹具使用独立输出目录，正常启动不注入这些故障。修改前对照源码及GDB断言阅读。
