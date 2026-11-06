# Limine 物理内存布局

Phase 3 请求并复制 Limine memmap 和 HHDM 响应，不假设固定 RAM 容量或固定 HHDM 偏移。
最多接收 256 个区域，检查 base+length 溢出。可用区向内取整到 4 KiB；非可用区向外取整，
并在第二遍覆盖可用标记，保留区优先。不连续区域之间的洞从来不进入空闲集合。

只有 USABLE 可分配。reserved、ACPI、bad、bootloader reclaimable、kernel/modules、framebuffer
全部保留，当前不回收 bootloader 页，因此 Limine 的页表、响应和其他引导数据继续安全使用。
元数据为 ELF 内的静态数组，自然属于 kernel/modules 保留区，不从空闲区“借用”后忘记预留。
物理页 0 额外禁止分配。异常/未知类型按保留处理。

当前明确支持可用 RAM 物理地址低于 4 GiB；超出上限拒绝初始化，不静默丢弃区域。
`memory_state.usable/free/allocated/tables` 单位为页，取自实际区域和状态转换，不是 QEMU 参数假数。
这里 usable 指经过边界/保留处理后的可管理完整页数，不包含半页和不可回收内存。
