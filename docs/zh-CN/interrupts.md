# 中断与异常

GDT 包含内核/用户段及占两槽的 TSS。RSP0 跟随当前内核栈，Double Fault 使用独立 16 KiB IST。IDT 共256门，只有 INT 0x82 向用户开放。汇编统一错误码、保存15个通用寄存器、清理DF/NT、对齐C栈，最后从目标帧 IRETQ 返回。PIC 将定时器/键盘映射到32/33，其余屏蔽；PIT 分频11932约100Hz，先EOI再调度。内核异常停机，用户异常结束任务。

[Source / 源码](../../kernel/arch/x86_64/isr-common.S) · [Index / 目录](index.md)

## 实践与验证

```sh
make test-phase2
```

观察真实客户机状态：irq_state / last_exception / DF IST / ABI / EOI. 测试需QEMU/GDB位于PATH或配置对应环境变量。故障夹具使用独立输出目录，正常启动不注入这些故障。修改前对照源码及GDB断言阅读。
