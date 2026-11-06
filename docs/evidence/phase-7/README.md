# Phase 7 原始文字证据

所有记录均为本次实际命令输出，不是预制的内核状态。目录时间为UTC，报告为Asia/Taipei。

- baseline/：开发前完整Phase1–6回归，调度5秒；baseline.log为顶层完整输出。
- regression-120s/：完整make test，256MiB调度120秒；regression.log为顶层完整输出。
- final-regression/：两次正常路径clean构建后的完整复验，调度5秒，包含新用户创建失败回滚。
- final-regression-first-failure/：新回滚夹具按64位比较C int返回值的失败；修正为32位EAX后重跑。
- integration-first-failure/：首次新增测试的过早稳态采样失败，未计入通过。
- integration/：五次BIOS、独立canary损坏panic和一次OVMF UEFI的GDB/串口/命令。
- four-ram-userspace/：四种RAM的真实CPL、权限、syscall和应用回收。
- lifecycle-30/：30轮额外Hello/Counter/echo，累计69次退出；measurements.json有逐次端到端计时。
- clean-first/second.log、.sha256：正常build路径两次实际clean构建。
- reproducible-first/second.log、.sha256：专用固定输出路径两次实际clean构建。
- expected-failure.log：故意CC=false的非零构建失败；不是正常构建失败。
- incremental-deps.log、source-integrity.txt：Makefile依赖与源输入指纹检查。
- changed-files.txt：最终修改文件清单；performance-summary.json：生命周期样本统计。

日志和GDB脚本会保留本次容器/临时socket路径；重新执行应使用tests/*.py生成当前路径。
QEMU命令JSON记录实际参数。未提交构建缓存、ELF/ISO、固件、socket或大截图。
独立panic和异常故障是预期的测试目标，不表示正常启动流程会触发它们。
