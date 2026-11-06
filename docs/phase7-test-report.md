# Phase 7 最终稳定化测试报告

日期：2026-11-07（Asia/Taipei；原始运行目录使用 UTC）。基线 main `7c3c921` 已合入 Phase6，
开发分支 `codex/phase7`。本次没有更换宏内核架构、开发通用 ELF 加载器或 HoofTTY 功能。

## 环境与执行方法

Debian 13.6 x86-64 云容器；GCC 14.2.0-19、GNU ld 2.44、Make 4.4.1、Python3、
QEMU 10.0.13（TCG、pc、qemu64、单 CPU、std VGA）、GDB 16.3，Limine v8.7.0 固定提交
`aad3edd370955449717a334f0289dee10e2c5f01`，每次构建检查仓库内 SHA256SUMS。
UEFI 使用 Debian OVMF `2025.02-8+deb13u1` 的非 Secure Boot CODE_4M 与新复制 VARS_4M。
工具本地覆盖仅在忽略的 config.local.mk/PATH，未提交绝对工具路径。

证据位于 [evidence/phase-7](evidence/phase-7/)。保留日志、GDB 命令/真实输出、串口和 JSON，
不提交 ELF/ISO、缓存、固件、临时 socket 或 PPM 大截图。截图内容仍由原测试实际检查。

## 改动与静态审查

核心修复清单、风险分级和全部核心模块检查见 [stability-review.md](stability-review.md)。
重点是 user_run 从空闲检查到发布/回滚的单核临界区、链接符号数值边界、应用构建依赖、
调度/回收失败串口诊断，以及已有文档/日志与实际 Ring3 功能同步。没有降低安全断言。
没有在此次动态测试中发现未处理的严重内核故障；静态竞态推断并不冒充已复现故障。

## 运行验证

| 项目 | 命令/条件与实际结果 |
|---|---|
| 开发前基线 | `KERNSEED_STRESS_SECONDS=5 make test`，Phase1–6 原测试全部通过；明确为快速调度配置 |
| 完整回归 | `make test`，原宿主 C、ELF、启动、CPU异常、内存、调度、键盘/Shell、Ring3 全部通过；256 MiB 调度区间采用默认120秒 |
| 最终快复验 | 两次正常路径干净构建后，`KERNSEED_STRESS_SECONDS=5 make test` 再执行全部阶段，新增用户创建失败回滚；此轮5秒不冒称120秒 |
| 启动 | 原64/128/256 MiB冷启动、GDB C入口/长模式/BSS、缺 framebuffer 预期 panic；新增5个独立 BIOS 256 MiB 冷启动全部通过 |
| 中断 | GDT/TSS/256 IDT项、软件 INT 的全GPR/DF恢复和 ABI；真实 #DE/#UD/#GP/#PF/#DF，DF 使用实际IST；IRQ0 ticks/EOI一致，无入口IF/DF违规 |
| 内存 | 128/256/512/1024 MiB 的PMM单/连续页、非法/重复/引用释放拒绝、实际耗尽恢复、逐页统计；VMM实际读写、冲突、解除、CR3/HHDM遍历、只读/NX/缺页故障全部通过 |
| 调度 | 非yield A/B抢占、64位GPR和DF/栈哨兵、主动yield/睡眠/唤醒/退出；创建中途失败回滚、16轮槽复用、容量拒绝；27退出/27回收，最后allocated=mapped=0，Idle实际IF=1/HLT |
| 持续调度 | 256 MiB 连续120秒，不暂停在压力区间；采样switches=3606、preemptions=2400、records=1200，register_errors=sched_state.errors=0。128/512 MiB另各5秒；这不是小时/天级耐久 |
| 栈边界和诊断 | 独立guard9/10实际写边界#PF；新增独立QEMU由GDB损坏A栈canary，真实IRQ检查输出scheduler panic，错误计数1、最终IF=0；正常镜像不自动损坏栈 |
| Shell | 真实QMP→PS/2事件执行help/about/clear/echo/meminfo/tasks/uptime/reboot/apps/run；引号/转义/空参数/空行/未知/参数超限/140字节长行、255队列背压、退格/Shift/鼠标端口、恢复和重启通过；屏幕像素/宿主滚屏检查通过 |
| 用户态四RAM | 128/256/512/1024 MiB 各独立启动，GDB断实际CS=2b/SS=33和用户RSP，再断int82的CPL0/内核栈；真实IRQ0切离CPL3。每次9次退出、4次故障、4次非法参数/编号，回收后allocated=mapped=16 |
| 用户权限 | Probe错误返回；用户cli实际#GP13/error0，内核读#PF14/error5，内核写#PF14/error7；NT返回标志和RSP=0安全终止，后续Hello/Shell可继续运行 |
| 创建回滚 | GDB利用已有fail_after=2夹具，在用户页已映射后让内核任务栈映射中途失败；user_run返回-1、active=0、allocated/mapped回到16且账目一致；随后正常Hello继续成功 |
| 内核页权限 | 新BIOS/UEFI测试独立读取真实CR3各级（含大页），_start为supervisor/RX、启动栈supervisor/RW/NX；不只检查ELF标志 |
| UEFI | OVMF 256 MiB 从同一混合ISO实际启动至Shell，CPU/PMM/VMM/PIT/调度及内核页权限检查通过；Secure Boot和其他固件未验证 |

### 多轮交互与资源

`KERNSEED_TEST_MEMORY=256 KERNSEED_APP_REPEATS=30 python3 tests/phase6.py`，
完整场景实测 **82.841秒宿主墙钟**（包含启动、调试暂停和键盘事件）。原9次用户退出后再运行
30轮 Hello + Counter + 带引号 echo，共69次用户退出、4次预期用户故障、124次真实Ring3抢占，
系统调用20933次，应用已退出且所有任务回收后 allocated=mapped=16，tables=5，账目一致。
没有增加数据页或栈占用；首次用户窗口建立比冷启动多一张保守保留的页表，随后稳定。
不能由这次有限运行证明所有输入或无限时长不存在泄漏。

## 性能/资源检查方法和实值

时间来自 Python time.monotonic，单位秒；QMP按键hold10ms、宿主逐字符间隔30ms，
计时包括字符输入、轮询、模拟调度和串口/Framebuffer输出。它们是端到端响应，不是纯内核延迟。
首次Hello的GDB特权级追踪会暂停CPU，故下面仅统计后续30轮未设置断点的重复命令。

| 项目 | 次数/条件 | 实际结果 | 限制 |
|---|---|---|---|
| BIOS进程启动到首个提示符 | 独立冷启动5次、256MiB | min0.8574 / mean0.8983 / max0.9581秒 | 含固件、PMM耗尽自检、模拟器启动/轮询；同机有构建任务，非内核初始化独占时间 |
| UEFI进程启动到提示符 | OVMF1次、256MiB | 2.3167秒 | 单样本、无Secure Boot，无泛化意义 |
| echo带引号参数 | 30次、256MiB | min0.5183 / mean0.6035 / max0.6249秒 | 包含逐字符输入和输出，无解析器微基准声明 |
| Hello运行至提示符 | 30次 | min0.5057 / mean0.5635 / max0.6077秒 | 包含装载、系统调用、退出、回收和键盘输入 |
| Counter运行至提示符 | 30次 | min1.2699 / mean1.2966 / max1.3248秒 | 程序主动运行至50个真实PIT tick；不是syscall开销 |
| 上下文切换 | 120秒压力1次、256MiB | 3606次切换/2400次抢占，错误0 | 记录实际事件吞吐，未测量一次切换的独立周期数 |
| PMM/VMM | 四RAM实际自检与连续应用生命周期 | 分配/解除/耗尽恢复及统计通过；稳态16数据页+5表页=84KiB | 不含内核ELF/BSS/HHDM开销；未可靠隔离单次alloc/free/map延迟 |
| syscall | 82.841秒场景 | 20933次实际调用，用户退出69次 | 包含应用工作、输入与GDB，不以总时长/调用数伪造单次延迟 |
| Idle | 调度器全部27次退出后 | allocated=mapped=0、current=0、IF=1、实际HLT | 不测宿主CPU占用百分比；默认A/B忙任务并非Idle负载 |

未测定内核微操作周期、最坏关中断延迟和精确CPU功耗；建立的方法是可重复工作负载、单调时钟、
GDB计数和资源快照。将来若要微基准，应另行校准目标CPU计时源并排除GDB/TCG暂停，不能套用这里的端到端数字。

## 干净构建、失败语义与可复现性

1. 保留原测试树到仓库外本地目录，然后正常路径 `make clean; make -j2 iso check` 两次。
   每次开始无build输出，保留已校验的固定Limine下载缓存和本地工具配置。两次ELF/ISO哈希完全一致。
2. `make reproducible` 在固定 `build/reproducible-clean` 路径另执行两次真实clean构建，哈希一致；
   原脚本make -B的不足已修正。该路径的生成汇编DWARF路径不同，因此不与正常路径哈希混为同一个结果。
3. `make BUILD=build/expected-failure CC=false iso` 实际返回2，未把失败当成功。
4. `make -n -W Makefile ...` 确认应用ELF/嵌入汇编会重建；脚本执行位、固定依赖校验、源码指纹和git diff检查通过。
   最终回归恢复默认镜像，重新核对哈希；LICENSE和第三方文件未变。

正常路径两次干净构建的最终SHA-256：

```text
367eafad2566bb508645a7f7c6997060067b28ae0c7ed1f56d2ea04348ac4558  build/kernseed.elf
64ef299a0d30381175e5212424f9bb083f574658d9b0555e1f38ae9bcd93164b  build/kernseed.iso
```

专用reproducible路径两次构建SHA-256：

```text
9761fe16bfd8ecd4dcb78521cce62ebadca918b17898c2a03d1bde3d09756eea  build/reproducible-clean/kernseed.elf
6b06d0b2f427fcec56c5292a8c69988509fbb4b6aed5c2cf456a86213295146c  build/reproducible-clean/kernseed.iso
```

结论限同一检出路径/工具链/设置；不是跨版本或跨路径完全一致承诺。主要构建输入来自源码和固定依赖，
没有把本地工具安装路径写进构建规则。

## 失败记录与未验证范围

新增集成测试首轮在Shell首个提示符后立即断言16页，但启动短任务尚未全部回收，断言失败。
这是稳态采样时序错误：改为GDB等待真实sched_state.reaped==2后在scheduler入口采样，
保留16页与统计断言；五次冷启动及UEFI后续均通过。首次失败日志也保留，不算通过。
新增用户回滚测试首轮也有夹具错误：GDB finish 已显示 C int 返回 -1，但断言按64位RAX的long比较；
ABI只规定32位EAX返回int，改为(int)$eax后重跑完整回归，仍要求active=0、16页和账目一致。
未改内核返回值或放宽失败回滚条件。首轮失败日志保留。
独立canary损坏和CC=false属于预期故障测试，与正常镜像失败区分。

本次云端未执行WSL2、VMware、KVM、真实硬件、SMP、其他固件/Secure Boot、小时/天级持续运行、
spurious IRQ7/15时序注入或所有大页组合。发起人报告Phase1–6已在WSL2/QEMU本地启动验收，
这不是本次Phase7亲自执行的测试。单活跃用户映像、固定ABI、无通用ELF/文件系统等限制保留。

完整修改清单见 evidence/phase-7/changed-files.txt；后续人工事项见 release-readiness.md。
提交标题 `Stabilize Kernseed OS Phase 7`，推送codex/phase7并尝试PR，不合并main、发布或归档。
精确提交及远程结果以最终交付消息为准。
