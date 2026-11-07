# 启动流程

Limine 按 ELF PT_LOAD 装载高半区内核，提供 framebuffer、内存图与 HHDM，使用 base revision 3。entry.S 执行 CLI/CLD，切换到 64 KiB 栈，以 SysV 对齐调用 kernel_main。BSS/C 自检之后依次建立描述符表、内存管理、PIC/PIT 和键盘；kernel_idle 创建任务后才 STI。BOOT_READY 不等于 Shell 已运行。

[Source / 源码](../../kernel/arch/x86_64/entry.S) · [Index / 目录](index.md)

## 实践与验证

```sh
make test
```

观察真实客户机状态：boot_state / BSS / CR0 / CR3 / CR4 / EFER. 测试需QEMU/GDB位于PATH或配置对应环境变量。故障夹具使用独立输出目录，正常启动不注入这些故障。修改前对照源码及GDB断言阅读。
