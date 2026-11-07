# 物理与虚拟内存

物理地址标识RAM，虚拟地址经CR3和四级页表转换，每级512项。HHDM提供物理RAM的内核虚拟别名，不代表所有权。PMM仅接纳完整USABLE 4KiB页框，保留区向外对齐，检查溢出，以状态字节和映射引用区分FREE/OWNED/TABLE；元数据约3MiB。VMM使用起点0xffffc00000000000的1GiB窗口，清零新表、拒绝冲突并刷新TLB。各级共同决定Writable/User/NX权限。可以查询大页但不拆分；数据页解除映射后可释放，空表保留。

[Source / 源码](../../kernel/memory/memory.c) · [Index / 目录](index.md)

## 实践与验证

```sh
make test
```

观察真实客户机状态：memory_state / PMM exhaustion / CR3 walk / NX faults. 测试需QEMU/GDB位于PATH或配置对应环境变量。故障夹具使用独立输出目录，正常启动不注入这些故障。修改前对照源码及GDB断言阅读。
