# Kernseed OS 许可证迁移审查

## 范围与授权依据

本次按维护者的任务指示，将 Kernseed OS 项目自有代码授权为 GNU GPL 第 3 版或任何后续版本
（`GPL-3.0-or-later`）。允许商业和非商业使用，但须遵守 GPL 的适用条件。
HoofTTY 是基于 Kernseed OS 的派生项目；本次不修改或推送 HoofTTY。

审查基线：`d0c2823db657c0fa492da74fcb9ffc3f0f89dc38`。
开始时工作区干净，`work` 分支与远端 `main` 的提交一致。
GitHub 仓库页面的 `isArchived` 为 `false`；没有绕过归档只读限制。

已检查完整的 20 个提交（非浅克隆）、作者和提交者、源码文件历史、版权/许可标识以及
`docs/codex-experiment.md` 的来源说明。作者记录仅有 Evan Windproof / Evan-Windproof，
使用 `wangxikai2012@gmail.com` 和 `wangxikai2012@outlook.com`；合并及网页提交的提交者还包括 GitHub。
仓库记录说明项目自有实现由 Codex 编写、发起人监督验收，无 Linux 内核源码、外部终端库或外部字体。
本次以这些来源记录及维护者的明确迁移指示为依据；Git 作者记录本身不构成额外的权利转让文件。
未发现其他贡献者、未披露的第三方来源或需要另行停止迁移的权属冲突。
没有增添、伪造或覆盖项目 Copyright 声明。

## 未标识文件与历史证据

以下活动文件原先没有 SPDX 标识，逐一追溯后才添加标识：

| 文件 | 引入提交 | 来源核查 |
| --- | --- | --- |
| `apps/linker.ld` | `fb98c9d` | 同一项目作者在 Phase 6 引入的用户应用链接布局 |
| `boot/limine.conf` | `b5a108d` | 同一项目作者在 Phase 1 引入的项目启动配置，非 Limine 实现源码 |
| `scripts/debug.gdb` | `b5a108d` | 同一项目作者在 Phase 1 引入的项目调试命令 |

`docs/evidence/` 和 `docs/reports/` 内的生成 GDB 脚本、日志、截图及阶段报告是历史证据，
其文件历史也来自同一项目作者；保持逐字节不变，不将历史运行时许可证字符串当成当前声明替换。
历史报告中的 PolyForm 叙述仅说明当时状态，不能用于判定当前项目授权。
不存在 `README.zh-TW.md`，不新建缺失的翻译；现有英文和简体中文 README 的许可语义一致。

## 第三方组件与正式许可证文本

- `third_party/limine/` 的三个跟踪文件全部保持逐字节不变。
- `third_party/limine/LICENSE`：BSD-2-Clause，保留 mintsuki and contributors 原版权及条款。
- `third_party/limine/limine.h`：0BSD，保留头文件内完整通知；不改为 GPL。
- `third_party/limine/SHA256SUMS`：保持不变；固定上游提交及下载后校验仍有效。
- 构建 ISO 继续携带独立的 `LIMINE-LICENSE`；工具不会被当作项目自有代码重新许可。

GNU 网站文本下载返回 HTTP 403，因此使用 GNU GCC 官方 GitHub 镜像的完整 `COPYING3`：
https://github.com/gcc-mirror/gcc/blob/5115c7e447fc07457443df874bf57840e8316d5f/COPYING3

根 `LICENSE` 与该文件逐字节相同：674 行、35147 字节，SHA-256：
`8ceb4b9ee5adedde47b31e975c1d90c73ad27b6b165a1dcd80c7c545eb65b903`。
未修改正文中的 Free Software Foundation 版权、许可条款、附录或示例。
项目选择“或后续版本”由 README 和自有源文件 SPDX 表达。

## 历史授权保留

迁移不撤销历史 Release 的原有许可，不删除或重写历史提交、Tag、Release。
原 PolyForm 正文仍可从基线提交的 `LICENSE` 读取，SHA-256：
`c0ea4a896d2c8c394b29f9427589996db826cd501c512279ff0ed3ef48fabbe5`。
原始文本来源记录：
https://github.com/polyformproject/polyform-licenses/blob/76a278c402bc43b8d2b561da140b0f3e17263015/PolyForm-Noncommercial-1.0.0.md

## 变更边界

活动源文件仅更换或补充 SPDX 注释；唯一的非注释源码变更是 `about` 命令显示的许可证名称。
不改变内核逻辑、架构、测试断言、构建配方或功能。
Makefile 仅更换 SPDX 注释；`.gitignore` 不变，构建输出仍位于忽略的 `build/` 和 `.cache/`。

## 验证结果

- 迁移前：`make -j2 iso check test-host`、`python3 tests/phase6.py` 均通过（128/256 MiB）。
- 迁移后：`make -j2 iso check test-host` 与完整 `make test` 均以退出码 0 完成。
  覆盖四组宿主测试、启动/GDB、异常注入、内存映射、调度器（保留默认 120 秒压力时长）、
  栈 guard、Shell 键盘/重启及 Ring3 应用/系统调用/故障。
- `make test-integration` 通过：五次 BIOS 冷启动、受控 panic、128/256/512/1024 MiB Ring3 矩阵。
  本次证据：`build/phase7-results/20261116T163533Z/results.json`；UEFI 未执行（可选 firmware 未配置）。
- `make reproducible` 通过：两次干净构建的 ELF 和 ISO 哈希一致。
- `about` 的实际串口输出包含 `GPL-3.0-or-later`。
- 对所有 77 个活动源文件/配置逐一按基线比较，仅允许 SPDX 注释与 `about` 许可证名称差异；检查通过。
- GPL 正文逐字节比较、Limine 校验和、第三方/历史证据文件不变检查、SPDX 一致性、
  `git diff --check`、现有双语 README 许可内容核对、忽略规则检查均通过。
- `.gitignore` 和构建配方不变；新测试产物仅留在忽略目录，不提交。

## 修改文件清单

- `LICENSE`
- `Makefile`
- `README.md`
- `README.zh-CN.md`
- `THIRD_PARTY_LICENSES.md`
- `apps/abi.h`
- `apps/badflags.c`
- `apps/badstack.c`
- `apps/counter.c`
- `apps/hello.c`
- `apps/kernelread.c`
- `apps/kernelwrite.c`
- `apps/linker.ld`
- `apps/privileged.c`
- `apps/probe.c`
- `boot/limine.conf`
- `boot/linker.ld`
- `docs/app-development.md`
- `docs/en/app-development.md`
- `docs/known-limitations.md`
- `docs/license-migration.md`
- `docs/shell-development.md`
- `docs/zh-CN/app-development.md`
- `include/kernseed/api.h`
- `include/kernseed/boot.h`
- `include/kernseed/console.h`
- `include/kernseed/interrupts.h`
- `include/kernseed/io.h`
- `include/kernseed/keyboard.h`
- `include/kernseed/memory.h`
- `include/kernseed/runtime.h`
- `include/kernseed/scheduler.h`
- `include/kernseed/shell.h`
- `include/kernseed/terminal.h`
- `include/kernseed/user.h`
- `kernel/arch/x86_64/counter-tasks.S`
- `kernel/arch/x86_64/entry.S`
- `kernel/arch/x86_64/gdt-load.S`
- `kernel/arch/x86_64/gdt.c`
- `kernel/arch/x86_64/interrupt-probe.S`
- `kernel/arch/x86_64/isr-common.S`
- `kernel/arch/x86_64/user-enter.S`
- `kernel/core/api.c`
- `kernel/core/console.c`
- `kernel/core/main.c`
- `kernel/core/runtime.c`
- `kernel/core/terminal.c`
- `kernel/drivers/framebuffer.c`
- `kernel/drivers/keyboard.c`
- `kernel/drivers/serial.c`
- `kernel/interrupts/interrupts.c`
- `kernel/memory/memory.c`
- `kernel/memory/span.c`
- `kernel/scheduler/scheduler.c`
- `kernel/syscall/user.c`
- `scripts/check-reproducible.sh`
- `scripts/debug.gdb`
- `scripts/fetch-limine.sh`
- `scripts/gen-isr.py`
- `scripts/run.sh`
- `scripts/setup-host.sh`
- `shell/commands/about.c`
- `shell/commands/apps.c`
- `shell/commands/clear.c`
- `shell/commands/echo.c`
- `shell/commands/help.c`
- `shell/commands/meminfo.c`
- `shell/commands/reboot.c`
- `shell/commands/run.c`
- `shell/commands/tasks.c`
- `shell/commands/uptime.c`
- `shell/dispatch.c`
- `shell/parser.c`
- `shell/registry.c`
- `shell/shell.c`
- `tests/boot.py`
- `tests/check_elf.py`
- `tests/framebuffer.c`
- `tests/memory-span.c`
- `tests/phase2.py`
- `tests/phase3.py`
- `tests/phase4.py`
- `tests/phase5.py`
- `tests/phase6.py`
- `tests/phase7.py`
- `tests/runtime.c`
- `tests/shell.c`
