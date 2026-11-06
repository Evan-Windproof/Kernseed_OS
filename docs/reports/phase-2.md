# Phase 2 阶段报告

日期：2026-11-06（Asia/Taipei）。范围：CPU 描述符、真实异常、中断入口与 PIT IRQ0。
本阶段在 Phase 1 基础上增量实现，没有重新生成内核项目，也没有实现 Phase 3–7。

## 开发基线与环境

- 从远程 `main` 的 `dab72f22b85d4eb6664caad6100781925db5e2e6` 建立 `codex/phase2`。
  该基线包含 Phase 1 合并和发起人的 README 更新，均已保留。
- 修改前完整执行原有 `make test`：全部通过；基线日志保存在本报告 evidence 目录。
- 实际机器为 Debian 13 x86-64 云环境，GCC 14.2.0 / ld 2.44 / Make 4.4.1，
  QEMU 10.0.13 TCG、SeaBIOS、GDB 16.3，单 CPU。复用 Phase 1 的本地工具配置。
- Limine 固定提交仍为 `aad3edd370955449717a334f0289dee10e2c5f01`（v8.7.0），
  协议 base revision 3。沿用高半区 ELF、启动栈和引导器页表。
- PolyForm Noncommercial 正文、第三方通知及 Limine 文件未修改。

## 实现功能

| 模块 | 实际行为 |
|---|---|
| GDT | CPL0 64 位代码/数据段、双槽 TSS 描述符；LGDT、远返回重载 CS、加载数据段 |
| TSS/IST | LTR；104 字节 TSS；RSP0；独立 16 KiB Double Fault IST 栈 |
| IDT | 256 个完整 64 位 interrupt gate，DPL0；向量 8 使用 IST1 |
| 汇编入口 | 区分 CPU 错误码；保存 15 个通用寄存器；CLD；C 调用对齐；恢复后 IRETQ |
| 异常 | #DE/#UD/#GP/#PF/#DF 等现场记录，输出 vector/error/RIP/RSP/CS/RFLAGS，#PF 附 CR2 |
| 致命错误 | IF=0 的独立 HLT 路径；嵌套诊断仅尝试有界串口，避免递归 framebuffer 输出 |
| PIC | 重映射 32–47、显式 EOI；master mask=0xfe / slave mask=0xff，只允许 IRQ0 |
| PIT | channel 0 mode 2，divisor 11932，约 99.9985 Hz；真实 tick 在 IRQ0 中递增 |
| 软件 INT | CPL0 的 0x80 内部自检，验证所有通用寄存器与 DF 恢复；不是 syscall ABI |
| 构建隔离 | 正常 ELF 不包含故障注入指令；5 类异常使用独立目录/镜像，默认 ISO 不受覆盖 |

初始化顺序是 CLI → 原有启动自检与控制台 → GDT/TSS → IDT → PIC 屏蔽与重映射 →
软件中断返回自检 → PIT 配置/仅解除 IRQ0 屏蔽 → BOOT_READY → idle 中首次 STI。
早期 panic 与致命异常调用 `cpu_halt`，不误用会启用中断的 `kernel_idle`。

## 实际测试结果

最终 `make test` 退出状态 0，结果与命令日志保存在 [evidence/phase-2](evidence/phase-2/)。
每个断言检查实际目标寄存器或内存，不能只凭文字输出通过。

| 检查 | 结果与证据 |
|---|---|
| Phase 1 宿主测试 | 12,464 个 memmove 组合、memcpy/memset 哨兵、framebuffer 字形/边界/拒绝输入通过 |
| ELF 检查 | 静态 ELF64，无未解析符号、动态加载器、RWX 段；通过 |
| 重复冷启动 | 64/128/256 MiB 三个独立 QEMU 进程，串口/屏幕/idle 正常；通过 |
| 旧 GDB/失败路径回归 | C 入口栈/BSS/长模式，及无 framebuffer 的预期 panic；通过 |
| GDT/TSS/IDT | GDB 解码全部 256 个 gate、5 个 GDT 槽和 busy TSS；核对实际 GDTR/IDTR/TR；通过 |
| 软件中断入口 | 原始 DF=1、非 16 对齐被中断 RSP；C 入口 RSP 模 16=8、IF/DF=0；所有 GPR 哨兵恢复；通过 |
| 真实 IRQ0 入口/返回 | 向量 32、合成 error=0、原始 IF=1；IRETQ 回到 idle 后 IF=1；通过 |
| tick 与 EOI | 间隔运行后两次 GDB 一致快照，tick 真正增长且与 EOI 相等，其他 IRQ=0；通过 |
| PIC 硬件配置 | QEMU `info pic` 核对实际 imr=fe/ff、vector base=20/28（十六进制）；通过 |
| #DE | 真正执行除零 `div`，vector=0/error=0，故障 RIP 与保存 RSP 正确；通过 |
| #UD | 真正执行 `ud2`，vector=6/error=0；通过 |
| #GP | 加载超出 GDT limit 的 0x28 选择子，vector=13/error=0x28；通过 |
| #PF | 读取未映射 canonical 地址，vector=14/error=0，CR2=0x400000000000；通过 |
| #DF | 使 #GP gate 不 present 后触发 #GP，投递失败导致真正 #DF；vector=8/error=0，现场在 DF IST；通过 |
| 各异常 ABI/停机 | 每类故障 C 入口对齐，错误码与栈帧准确；最终 QEMU HLT=1、IF=0、tick=0；通过 |
| 正常镜像隔离 | 默认 ELF 无 fault 指令符号；故障构建前后默认 ISO 哈希相同；通过 |
| 构建参数保护 | 未指定独立 BUILD 的 `make TEST_FAULT=1 iso` 按预期拒绝；通过 |
| 手工调试流程 | `make debug` + `scripts/debug.gdb` 本机 TCP，继续到初始化完成的 kernel_idle；通过 |
| 可重现构建 | `make reproducible` 两次强制重建 ELF/ISO 哈希相同；通过，限相同目录与工具链 |

精确计数器读数、GDB 输出、原始 CPU/PIC 状态、串口日志、QEMU guest_errors 和构建哈希均在 evidence。
最终正常测试中 tick/EOI 两次读数为 113/113 → 225/225，其他 IRQ 和 fatal 均为 0。
完整运行目录为 `build/test-results/20261106T052820Z/` 和 `build/phase2-results/20261106T052834Z/`（UTC）。
测试有意暂停/继续 vCPU，不能把采样墙钟间隔当作 PIT 精度校准。重复启动是短期稳定性检查，
不是长时间耐久测试。正常镜像的实际屏幕见 [screen.png](evidence/phase-2/screen.png)。

## 调试与设计修正

- 先完整复测 Phase 1，核实 CPU 已处于长模式、分页开启、IF=0，未引入额外 bootloader。
- 正常 idle 需要 STI，但 panic 绝不能启用尚未准备好的 IRQ；新增独立 CLI/HLT 终止路径。
- 使用生成器明确列出带错误码向量，避免手写 256 份重复入口出现差错。
- 采用对齐后 call、保存帧指针后恢复的策略，既兼容普通栈，也兼容 IST 与非对齐原始 RSP。
- GDB 原始 flags 类型与地址寄存器延续 Phase 1 已验证的读取方法，逐类检查实际 C ABI。
- HLT 截图检查允许短暂捕获 ISR，要求随后返回 idle；计数器在 idle 断点读取，避免 tick 已增而 EOI 尚未写入的采样竞态。
- PIC 的 ISR/IRR 可能随正在执行的 IRQ 改变，因此配置检查不误把瞬时 in-service 位当永久故障。
- 正常启动信息明确标注 **initial IF=0**；最终 IF=1 由 GDB 的真实 IRQ 返回检查证明，避免静态屏幕文字误导。
- 本阶段实际验收没有未解释的 QEMU 引导失败。故障镜像的异常诊断和停机是预期结果，不算普通启动成功。

## 修改文件清单

完整逐文件清单见 [changed-files.txt](evidence/phase-2/changed-files.txt)。主要变更：

- 新增 `include/kernseed/interrupts.h`：描述符、硬件栈帧与诊断接口。
- 新增 `kernel/arch/x86_64/gdt.c`、`gdt-load.S`、`isr-common.S`、`interrupt-probe.S`。
- 新增 `kernel/interrupts/interrupts.c`：IDT/PIC/PIT、异常终止与隔离故障选择。
- 新增 `scripts/gen-isr.py`、`tests/phase2.py`；生成的 stubs 位于 build，不提交。
- 更新 `entry.S`、`kernel/core/main.c`；保留 Phase 1 控制台/runtime/链接脚本/引导配置。
- 更新 Makefile 和 `scripts/run.sh`：兼容已有目标，增加独立 BUILD 与参数 stamp。
- 更新 `tests/boot.py`：保留既有回归，主入口保护允许复用 QMP 工具，兼容周期性唤醒的 idle。
- 增量更新 README、架构/启动/中断/GDB/QEMU/实验文档，新增本报告及真实运行证据。

## 启动与复测

```sh
make iso
make headless       # 或 make run（需要图形后端）
make debug          # 第二个终端：gdb -x scripts/debug.gdb
make test
make test-phase2
make reproducible
```

手动观察页故障：`make BUILD=build/fault-4 TEST_FAULT=4 headless`。
回到正常启动：`make headless`。除错镜像为受控停机用途，不是普通发布镜像。
默认镜像 `build/kernseed.iso`，带符号 ELF `build/kernseed.elf`。

## 未解决问题与范围限制

没有已知的本阶段验收阻塞；以下能力明确未交付或未验证：

- 无 APIC/SMP、用户态隔离、异常恢复、按需分页或任务调度。
- 无键盘/Shell、系统调用；软件 INT 自检不可作为已实现 Phase 6 的依据。
- IST 栈没有页级 guard；GDT/IDT 初始化前、IST/页表自身损坏时不保证可诊断，更不保证任意情况下无 triple fault。
- NMI/机器检查与所有扩展 CPU 异常没有逐一注入；IRQ7/IRQ15 spurious 分支没有自动时序注入验收。
- 无长期漂移校准/耐久测试；当前是 QEMU PC 单 CPU 的教育实现。
- 本次 Phase 2 未在 WSL 2、UEFI、实机或 KVM 上执行验收，不把历史 Phase 1 本地启动当作 Phase 2 验证。

## Git 交付记录

开发分支为 `codex/phase2`，基线为上述 `dab72f2`；阶段提交标题为
`Add Kernseed OS Phase 2`。精确提交 SHA、推送及 PR 创建结果由交付消息给出，
可使用 `git log --oneline dab72f2..codex/phase2` 复核。不合并或覆盖 main，不发布或归档仓库。
