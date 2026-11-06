# 启动流程

1. QEMU PC/SeaBIOS 从 ISO El Torito BIOS 启动项加载 Limine。
2. Limine 读取 `boot/limine.conf`，按 ELF64 PT_LOAD 装载内核，建立 x86-64 长模式环境
   和启动页表，填写 framebuffer 响应。请求固定为 base revision 3。
3. ELF 入口 `_start` 执行 `cli; cld`，将 RSP 切到内核 `.bss` 中 64 KiB 栈顶。
   栈顶 16 字节对齐，`call kernel_main` 后入口 RSP 模 16 等于 8，符合 SysV ABI。
4. 内核初始化 COM1，通过 UART loopback 检查硬件响应，输出早期诊断。
5. 检查 Limine revision、256 字节 BSS probe 和 C 内存函数自检。
6. 验证 framebuffer 格式并清屏，自有 5×7 字体按 2× 像素显示；小写显示为大写。
   支持 32bpp RGB、互不重叠且字节对齐的 8 位色彩通道。其他格式明确 panic。
7. 读取 CR0/CR3/CR4、EFER 和 RFLAGS，检查 paging/PAE/long mode 与 IF=0。
8. 输出真实 framebuffer 尺寸和 CR3，安装自有 GDT/TSS/IDT，验证 GDTR/IDTR/TR。
9. 执行 CPL0 软件 INT 寄存器与 DF 恢复自检；只有隔离故障镜像才执行危险异常。
10. 初始化 memmap/HHDM、PMM、受控 VMM，执行真实分配/映射/耗尽自检；独立 Phase 3 故障镜像此时触发页故障。
11. 配置 PIC/PIT，仅放行 IRQ0，仍不执行 STI。输出 BOOT_READY，设置 `boot_state.stage=2`。
12. 进入 `kernel_idle`，此时才 STI，HLT 等待实际定时器中断，ISR 通过 IRETQ 返回。

正常路径周期性被 IRQ0 唤醒，再回到 HLT；tick 由 PIT 中断递增而非空闲循环次数。
尚无任务调度。初始化中或不可恢复异常使用独立 `cpu_halt`，其 IF=0，不能误入启用中断的 idle。

早期失败路径设置 `boot_state.stage=0xff`，输出 `PANIC` 原因并关闭中断停机。
CPU 异常保存 `last_exception` 和 `irq_state`，输出现场并受控停机，详见 [interrupts.md](interrupts.md)。
无 framebuffer 时串口仍可诊断；串口等待有上限。GDT/IDT 初始化之前发生的早期异常仍无法保证捕获。

## C 与 ELF

链接基址 `0xffffffff80000000`。请求、代码、只读数据、可写数据分别使用独立 PT_LOAD，
无动态链接、无 libc、无可写可执行 ELF 段、无可执行 GNU 栈要求。实际页表来自 Limine，
ELF 段标志不能代替未来的页表权限测试。

编译禁用 red zone、栈保护器、PIE、浮点/SIMD 寄存器生成与宿主内建内存函数，
以 `-ffreestanding -mcmodel=kernel` 生成 x86-64 代码。当前不依赖浮点状态初始化。
`memcpy/memmove/memset/memcmp` 自行实现，不依赖宿主 C 库。

测试会实际断在 C 入口检查栈、DF/IF、BSS，再在 idle 前检查诊断数据；不是仅检查日志文字。
