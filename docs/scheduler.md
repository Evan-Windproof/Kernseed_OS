# 单核抢占式 Round-Robin 调度器

默认SCHEDULER=1。固定TCB表的READY槽按循环索引构成有界就绪队列；每次从当前槽之后扫描，
跳过UNUSED/BLOCKED/TERMINATED。最多8槽，无动态队列节点，算法O(8)，没有优先级和SMP。

PIT约100 Hz；当前任务时间片5 tick。每个IRQ0先计真实tick、EOI，再进入调度检查。
时间片结束将RUNNING改READY，选择下一READY任务，恢复其实际中断帧。
若只有一个普通任务，就继续它；没有普通READY任务则选择独立Idle。Idle执行STI/HLT并由IRQ0唤醒。
运行tick、抢占、切换、yield、退出、回收与记录次数都由真实执行路径累计。

A与B各在独立栈运行汇编计数循环，不主动yield；C每次记录真实A/B计数与tick，然后sleep(10)，
不会并发写Framebuffer。另有短任务验证主动yield和入口返回退出。
无Shell的隔离调度夹具设置demo_stop后，A/B返回，C执行16轮创建/退出/回收复用及TCB容量拒绝检查，随后退出；默认Shell模式的C直接退出，不运行容量夹具。
这是显式测试停止路径，普通运行demo_stop=0时三个演示任务持续调度。

scheduler_start在原kernel_idle入口之后、STI之前建立TCB。旧阶段精确验证“尚未分配任务栈”的
测试使用SCHEDULER=0兼容夹具；make test先完整运行这些检查，再重建默认SCHEDULER=1镜像，
执行真实调度集成测试。旧断言没有被删除或伪装为抢占测试。

IRQ关闭时不抢占PMM/VMM/队列临界区；普通任务无须主动yield才能被切走。
不支持在任意IRQ/NMI中创建阻塞操作、持锁sleep、优先级继承或跨CPU TLB同步。
长时间测试的实际时长与未覆盖场景见phase4-test-report.md，不把短期稳定当永久可靠。

Phase 7：调度约束失败先 CLI，输出有限串口 panic 再停机；GDB 可检查 sched_state/tasks。隔离 canary 损坏测试见 phase7-test-report.md，未把 guard 当任意损坏栈恢复机制。
