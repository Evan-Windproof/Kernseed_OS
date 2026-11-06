# Virtual Memory Manager

继续使用 Limine CR3，不重建地址空间。初始化验证四级模式（拒绝 LA57/PCID）、CR0.PG/WP、
EFER.NXE，保存实际 CR3 和 HHDM。查询支持 4 KiB、2 MiB、1 GiB 叶子，返回物理地址及有效
Present/Writable/NX（祖先 Writable 取交集，NX 取并集）。非 canonical 地址拒绝。

修改权限仅限 `VM_BASE=0xffffc00000000000` 起 1 GiB。初始化要求对应 PML4 槽原先为空，
否则拒绝；不抢占内核或 HHDM 槽。映射必须页对齐，目标必须是 PMM OWNED 页，flags 仅接受
VM_WRITE/VM_NX（Present 自动添加）。重复映射不覆盖，物理引用计数溢出也拒绝。

缺失中间表从 PMM 分配为 TABLE，经 HHDM **清零后**才链接。遍历只进入自己拥有的子表，
不进入继承子树、不拆分任何大页。操作遇到大页返回失败；`vmm_error` 对 map 给出 INVALID、
OWNERSHIP、CONFLICT、HUGE、OOM 或 OK，可由 GDB 检查。query/unmap 使用 bool 返回值。

解除映射先清 PTE、INVLPG，再减物理引用；不自动释放数据页，调用者之后执行 pmm_free。
不会把还被另一受管别名映射的页重新分配。查询可以访问继承映射，但修改不行。

保守策略：空中间页表当前不回收；分配中途失败时已建立的空表仍被标为 TABLE、计入统计，
绝不错误返回空闲池。1 GiB 控制区最多需要 514 张中间表；这是明确的空间限制，不是完整通用 VMM。
没有用户态地址空间、COW、换页、TLB shootdown、页表共享或大页拆分。
