# Codex开发实验

Codex实现内核模块、构建脚本、QEMU/GDB测试与文档。发起人制定范围、审查合并、决定发布，并报告QEMU/VMware验收。自动测试检查真实寄存器、栈帧和页表，启动成功不足以证明正确。Phase7记录120秒抢占、四种RAM和重复应用回收；这些是历史实测，不是本轮重跑。过早稳态采样、int返回宽度两次夹具错误已修正并保留失败日志。分支可推送而PR GraphQL返回Forbidden；本轮Release元数据查询也Forbidden，因此不编造版本号。这是有限实验，不证明AI普遍完全自主开发内核。

[Evidence](../evidence/phase-7/README.md) · [Report](../phase7-test-report.md)

[Index](index.md)
