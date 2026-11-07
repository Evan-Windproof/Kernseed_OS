# 任务与抢占

任务是执行上下文，不一定是隔离进程。8个TCB槽包含Idle；普通任务使用独立4页NX内核栈，带未映射guard和canary。READY进入RUNNING；yield/时间片到期返回READY，sleep进入BLOCKED直到唤醒，退出进入TERMINATED。IRQ0结束5tick时间片，通过isr_common恢复另一任务帧；INT0x81是内部调度trap。另一栈执行延迟回收。A/B不yield并检查64位寄存器、DF与栈哨兵；C记录实际进度。无普通就绪任务时Idle执行STI/HLT；默认忙任务会消耗CPU。

[Source / 源码](../../kernel/scheduler/scheduler.c) · [Index / 目录](index.md)

## 实践与验证

```sh
make test-scheduler
```

观察真实客户机状态：tasks / sched_state / register_errors / stack guards. 测试需QEMU/GDB位于PATH或配置对应环境变量。故障夹具使用独立输出目录，正常启动不注入这些故障。修改前对照源码及GDB断言阅读。
