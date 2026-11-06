# 双语文档重构记录

基线：main 938586b，已合入Phase7。分支codex/docs-bilingual。
保留全部旧技术文档、阶段报告和原始证据；仅替换默认README，新增中文主页及docs/en、docs/zh-CN。
不合并或删除历史资料，旧链接继续存在；新教学入口提供语言分离导航。

新增章节覆盖：入门、架构、启动、中断、内存、调度、终端/Shell、命令参考、命令扩展、系统调用、
用户态、应用开发、QEMU、VMware、GDB、阶段历史、Codex实验及限制。历史章节链接各阶段完整报告，
保留其目标、难点、失败和验收边界；不将历史测试冒充本轮重跑。

Release查询gh release list返回GraphQL Forbidden，无法独立核对tag/版本/资产。
正式发布及VMware基本验收仅归因发起人报告，本轮未亲自运行VMware。
截图沿用仓库Phase1引导、Phase5 Shell真实截图；缺少最终用户态/VMware截图，明确说明而不补造。

验证：make iso check test-host；现有QEMU test-shell；双语文件集合、Markdown相对文件链接、
Makefile目标、代码示例编译与内核/许可证无修改检查。详细检查结果以docs/reports/bilingual-checks.txt为准。
未重做Phase7全套压力测试；没有修改内核、应用、构建规则、正式许可证或第三方文件。
