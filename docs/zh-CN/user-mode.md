# Ring3执行

user_enter构造CS=0x2b、SS=0x33并以IRETQ进入Ring3，使用独立用户栈。代码位于0xffffc00001000000的一页User/RX，栈位于0xffffc00001100000的一页User/RW/NX。内核/HHDM保持Supervisor。共享CR3且仅一个活跃用户映像；门入口通过TSS.RSP0切到任务内核栈。检查危险flags/RSP，用户异常记录RIP/error/CR2后仅终止用户任务，延迟回收用户页与内核栈。不提供多个用户进程的地址空间隔离。

[Source / 源码](../../kernel/syscall/user.c) · [Index / 目录](index.md)

## 实践与验证

```sh
make test-integration
```

观察真实客户机状态：CS / SS / TSS.RSP0 / user_state / page permissions. 测试需QEMU/GDB位于PATH或配置对应环境变量。故障夹具使用独立输出目录，正常启动不注入这些故障。修改前对照源码及GDB断言阅读。
