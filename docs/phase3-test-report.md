# Phase 3 实际开发与测试报告

日期：2026-11-06（Asia/Taipei）。基线：最新 main `62945fd`（已合入 Phase 2），分支 codex/phase3。
开发前阅读 README、Makefile、架构、链接布局、异常入口与 Phase 2 报告，完整基线回归通过。
实际执行环境仍为 Debian 13 / GCC 14.2 / QEMU TCG SeaBIOS / GDB 16.3，非 WSL 本机验收。

## 实现与安全边界

新增 Limine memmap/HHDM 请求，复制区域信息，64位区间检查，区分全部类型；仅完整 USABLE 页进入池。
采用状态字节代替压缩 bitmap，以便区分保留、空闲、调用者分配与页表所有权。元数据静态放在内核映像中。
提供单页/连续物理页分配、单页释放与分配状态查询、引用保护和实际页数统计。

沿用 Limine CR3/HHDM，检查四级分页、WP/NXE，支持虚实查询（含大页叶子）、受控区4 KiB映射/解除、
Writable/NX、冲突拒绝、表页清零、INVLPG、OWNED引用计数。禁止修改继承子树，禁止释放TABLE和有引用的页。
Page Fault输出增加读/写、数据/指令与缺页/权限类型，原始CR2/error/RIP/RSP保留。

限制：可用RAM必须低于4 GiB，memmap最多256项；修改仅限独立1 GiB虚拟区，必须取得原先空的PML4槽。
不拆大页，不重建所有页表；空中间表和失败操作留下的已链接空表保守保留、计入tables，最多514页。
当前元数据开销3 MiB。无SMP/NMI分配支持、用户空间、按需分页、异常恢复或页表回收。
HHDM永久别名不算拥有引用；物理指针由可信内核调用者遵守生命周期，不能访问已释放页。

## 实际执行结果

最终完整命令 `make test` 成功，日志：`evidence/phase-3/acceptance.log`。
Phase 1/2 原有全部宿主、ELF、冷启动、GDT/IDT/TSS、真实异常、IRQ0/EOI、GDB与屏幕测试通过。

新增测试：

- 单页/连续3页分配、对齐、真实状态转换与分配后不重用；释放后统计恢复。
- 非对齐/页0/CR3保留页释放拒绝，重复释放拒绝，映射持有引用时释放拒绝。
- 穷尽实际空闲池，返回0及超大分配失败，随后只释放自检自己的OWNED页并核对统计。
- 区间纯函数测试：不对齐可用/保留、零长度、整页、64位上界和溢出。
- HHDM虚实查询，新4 KiB映射的偏移查询，实际虚拟写入/物理别名读取，解除后查询失败。
- 映射冲突、非canonical和不对齐地址拒绝；解除不存在映射拒绝。
- 128/256/512/1024 MiB分别真实冷启动，通过GDB逐字节计数PMM状态，并独立遍历CR3/HHDM下的三层新增表，验证最终叶表清零。
- 同时验证四种配置的PIT真实tick/EOI继续增长，未出现其他IRQ或fatal。
- 三类隔离#PF：解除映射后读（error=0）、只读页写（error=3）、NX页执行（error=17）。GDB检查CR2、RIP、错误码和最终IF=0/HLT。
- `make reproducible` 比较同目录/工具链两次强制重建，结果及SHA-256保存在证据目录。

启动内存自检是一次性、无既存OWNED分配时执行的自检，不是任意时刻可以调用的清理函数。
耗尽过程中不写页数据，不释放保留/内核/表页，不制造破坏内存的失败。
全部结果为真实运行；既有Phase2故障镜像在内存初始化前执行其原始隔离测试。

## 发生的问题

一次回归运行中，我同时手动重建了正常ISO，触发Phase2末尾的正常镜像哈希保护断言。
这是测试编排错误，不是故障子构建覆盖默认目录。保留失败日志，冻结源码后顺序重跑完整测试，断言通过；
未删除或放宽该检查。最终重新执行的日志与首次失败日志都保留。

没有已知阻止上述验收的内核错误。未验证UEFI/实机/WSL、SMP、大于等于4 GiB可用物理RAM、长期压力或1 GiB大页实机配置。
支持大页查询的算法不等于已经测试所有硬件大页组合。没有将本阶段描述为完整进程内存隔离。

## 源码、运行与交付

主要新增：include/kernseed/memory.h、kernel/memory/memory.c、span.c、tests/phase3.py、tests/memory-span.c。
更新Makefile、main初始化、PF诊断、Phase2测试主入口保护/内存参数，以及README和教学文档。
许可证、第三方文件、原有宏内核架构和链接布局保持不变。

```sh
make iso
make headless # 图形模式 make run
make debug   # 另一个终端 gdb -x scripts/debug.gdb
make test
```

故障6/7/8使用独立BUILD目录，不修改普通启动：
`make BUILD=build/fault-7 TEST_FAULT=7 headless`。
GDB可观察memory_state、vmm_error、CR3、CR2、last_exception；详细原始脚本和结果在evidence/phase-3。
Git阶段提交标题为 `Add Kernseed OS Phase 3`，推送与PR结果以最终交付消息为准；不合并main。
