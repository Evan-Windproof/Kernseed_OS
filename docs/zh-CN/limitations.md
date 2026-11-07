# 限制与安全

仅单核；可用RAM低于4GiB、最多256个内存区域；VMM窗口1GiB且保守保留空表。无FPU/SIMD/TLS上下文、SMP、通用ELF加载器、文件系统、动态链接、POSIX或Linux ABI。仅内置固定映像、一个活跃Ring3任务，代码/用户栈各一页。同步run没有外部kill，无限应用可能占住Shell，但其他任务仍可抢占。不可用于生产或重要数据，未做全面安全审计，不保证实机运行。

[Source / 源码](../../docs/known-limitations.md) · [Index / 目录](index.md)
