# Phase 6 实施与实际测试报告

日期：2026-11-07（Asia/Taipei）。基线main `830aaf2`已合入完整Phase 5，开发分支codex/phase6。
实际环境：Debian13、GCC14.2、QEMU TCG/SeaBIOS单核、GDB16.3。开发前执行Phase1–5回归通过。
调度压力采用明确5秒快速配置；本阶段不冒称120秒耐久或WSL/实机验收。

## 核心目标与边界

Kernel API整理到api.h/api.c：输出、内存/任务快照、真实tick和退出，聚合既有任务API。
新增DPL3 INT 0x82、用户段、Ring3代码/用户栈、逐任务TSS.RSP0与完整硬件中断帧返回。
sys_write/uptime/yield/exit是真正的用户→内核调用，不是把内核函数重命名。
检查编号、长度/溢出、用户范围、每级User权限、物理页归属；非法地址返回-14，未知编号-38。
危险NT/VM/IOPL等标志过滤，恶意RSP终止任务，不让内核调度栈断言停机。
用户GP/PF终止当前任务，其他栈延迟回收用户页和任务内核栈，Shell继续运行。

## 进阶目标明确未完成项

**未实现通用ELF加载器。** 用户程序独立编译为静态ELF，但构建时转平坦映像，运行时只加载内置白名单。
有大小检查、页清零、RX/NX、入口/用户栈及安全退出；没有ELF头/段解析、PT_LOAD、多段BSS、relocation。
只有一个活跃用户任务，共享内核CR3中受权限保护的固定窗口，不提供多个用户进程间的隔离或并发加载。
不是Linux ELF ABI，无POSIX/libc/动态链接/文件系统/FPU/SIMD/TLS/SMAP完整加固。

## 实际验证

完整make test通过Phase1–5全部原检查，并执行新增128/256 MiB用户态测试。
旧阶段以USERMODE=0保留精确GDT/IDT形状夹具，随后恢复默认USERMODE=1，真实CPL测试不可省略。
最终用户态复核和构建日志均在evidence/phase-6。

- 独立编译Hello/Counter/Probe和权限测试程序；ELF输出保留，但不把工具链ELF检查冒称内核ELF解析。
- GDB硬件断在用户代码，**实际CS=0x2b、SS=0x33**，RSP位于独立用户栈；再断在syscall_dispatch，
  **实际CS=8、保存帧CS=0x2b**，实际RSP位于当前任务内核栈，验证C ABI对齐与IF/DF。
- Hello通过真实系统调用输出指定文字；Counter查询实际tick、计算、被IRQ0真正抢占并yield/退出。
- Probe验证未知编号、NULL、内核地址、跨越栈页、零长度write；错误值严格检查，不靠预期文字替代参数验证。
- 用户cli实际#GP(vector13/error0)；读内核地址#PF(error5)、写内核地址#PF(error7)；分别由GDB核对现场记录和回收。
- 恶意NT返回标志、RSP=0的独立程序测试通过，未导致内核panic。
- 重复Hello和其他应用共9次退出，包含4次用户故障；全部回收后allocated=mapped=16，与Shell/演示内核任务原占用一致。
- GDB要求user_state.preemptions>0，该字段只统计真正从Ring3切走的IRQ0抢占，不以IRQ访问或普通循环数冒充。
- apps/run是独立注册命令，不修改Shell核心解析；原help/meminfo/tasks/uptime/reboot及PS2溢出/屏幕测试通过。
- 同目录/工具链两次强制重建的结果和SHA-256见证据；未宣称跨环境一致。

## 发现与修复

初始脚本替换把映射参数检查误插到pmm_free，编译报错，已修复并保留日志，无虚构成功构建。
原Counter固定计算太快，未耗尽时间片；将“真正Ring3抢占”的断言保留，改为运行至实际PIT经过50 tick。
诊断字段也区分IRQ到达和真正切走，避免把前者报告成抢占成功。
修正应用TCB名称指向临时Shell行缓冲的生命周期问题，改用静态名称。
安全审查补充用户RSP和危险返回标志验证；没有删除旧检查以让测试通过。

## 修改清单和运行

完整清单见evidence/phase-6/changed-files.txt。
主要新增api/user接口、kernel/syscall/user.c、user-enter.S、独立apps源码/ABI/链接脚本及phase6.py。
更新自有GDT/IDT、受控VMM User位、调度器TSS/用户栈检查与延迟回收、Shell快照API及apps/run注册命令。
许可证和第三方声明保持不变。

```sh
make iso
make run
# QEMU键盘：apps / run hello / run counter
KERNSEED_STRESS_SECONDS=5 make test
```

syscall ABI见syscalls.md，Ring3约束见user-mode.md，加载限制见program-loader.md，完整程序示例见app-development.md。
没有已知阻止上述核心验收的错误；通用ELF加载、多个隔离用户地址空间、无限循环程序的外部kill为明确未完成能力。
Git提交标题Add Kernseed OS Phase 6，不直接合并main；精确提交与推送/PR结果以最终消息为准。
