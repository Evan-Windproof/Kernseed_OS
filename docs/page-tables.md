# x86-64 四级页表与页故障

虚拟地址位 47:39、38:30、29:21、20:12 分别选择 PML4/PDPT/PD/PT 的 512 项之一，
低12位为4 KiB页偏移。CR3给出根表物理地址，表项地址字段仍然是物理地址；内核必须经实际
HHDM偏移访问，不能把表项的物理地址直接解引用。

PD/PDPT的PS位分别表示2 MiB/1 GiB叶子；查询按相应页尺寸计算偏移，不能把它误认为下级表。
修改路径明确拒绝PS，不尝试拆分大页。PTE权限含Present、Writable及NX，当前不提供User映射。
CR0.WP使内核态写只读页也产生权限故障；EFER.NXE使NX生效。

解除映射用INVLPG刷新当前CPU对应页；当前仅单CPU，不提供跨CPU同步。
Phase 2异常入口保留不变；#PF输出CR2、错误码、RIP/RSP，并解码：bit0表示protection/not-present，
bit1表示write/read，bit4表示instruction/data。原始错误码保留供其他位分析。
当前无缺页恢复，所有页故障受控停机。故障6/7/8独立镜像分别验证解除映射、只读写入和NX执行。
