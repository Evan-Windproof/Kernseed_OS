# Phase 7 验收与功能冻结

本阶段只稳定现有系统；没有发布 Release、上传 ISO、合并 main 或归档仓库。
具体执行记录见 [测试报告](phase7-test-report.md)，风险分类见 [审查](stability-review.md)，
交付边界见 [已知限制](known-limitations.md)。所有原始检查脚本随源码交付。

| 阶段 | 冻结状态 |
|---|---|
| 1 | 独立 Limine 引导、freestanding C、Framebuffer/串口、QEMU/GDB |
| 2 | GDT/TSS/IDT、真实异常、DF IST、PIC/PIT |
| 3 | 4 KiB PMM、受控四级 VMM、权限、HHDM 和 Page Fault |
| 4 | 独立内核栈、IRQ0 真抢占 RR、退出/延迟回收、Idle |
| 5 | PS/2、终端、模块化命令注册表与参数解析 |
| 6 | Kernel API、真实 INT82/Ring3、独立构建内置映像；通用 ELF 加载器未实现 |
| 7 | 有限代码审查、必要修复、完整回归、多环境/持续运行、干净构建哈希与文档同步 |

## 复验命令和构建条件

在 x86-64 Debian/Ubuntu 或 WSL2 Ubuntu 使用 GCC/GNU ld/objcopy、Make、Git、Python3、xorriso、QEMU x86、GDB。
`sudo sh scripts/setup-host.sh` 提供包列表；第一次 `make deps` 需访问 GitHub，校验固定 Limine 提交/文件 SHA256。
GUI 另需图形后端/WSLg。UEFI 测试可选安装 `ovmf`；不捆绑固件或从个人电脑读取绝对路径。
`config.local.mk` 是忽略的本地工具路径覆盖，不属于源码交付。

```sh
make deps
make -j2 iso check
make test                         # 默认含120秒调度；返回默认Shell/Ring3镜像
make test-integration              # 五次BIOS冷启动、独立panic、四RAM的Ring3应用
# 可选，在上面命令前设置 KERNSEED_OVMF_CODE / KERNSEED_OVMF_VARS
KERNSEED_TEST_MEMORY=256 KERNSEED_APP_REPEATS=30 python3 tests/phase6.py
make reproducible                 # 专用固定输出树，两次clean构建；不清除回归日志
make run                          # QEMU窗口：help / apps / run hello
make headless                     # 串口观察，非UART输入Shell
make debug                        # 另终端 gdb -x scripts/debug.gdb
```

直接执行 Python 测试时 QEMU/GDB 必须在 PATH 或通过 QEMU/GDB 环境变量指定。
已有 `make clean` 仍清理整个 build；reproducible 专用目录是 build/reproducible-clean。
已有 run/headless/debug/test/iso/clean 接口保留，新增 test-integration。
脚本失败返回非零；不要把预期故障镜像停机或编译成功等同全功能通过。

## 人工发布前事项（后续阶段）

发起人审查本分支/PR、执行最终 WSL2 验收，接受已记录限制与非商业许可；随后再决定合并、
双语 README/教学指南、示例软件、正式版本 ISO/Release 和归档。这里没有预先执行这些操作。

确认项包括：Phase7 日志结果与范围、未验证平台、固定内置用户映像而非通用 ELF、
应用和任务资源回收、许可证正文及第三方声明无意外变动、不提交缓存/镜像/本地配置。
构建可复现结论限同一路径/工具链，不承诺不同 GCC、主机路径或固件的字节一致。
