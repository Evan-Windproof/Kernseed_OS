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
8. 输出真实 framebuffer 尺寸和 CR3，设置 `boot_state.stage=2`，进入 `kernel_idle`。

正常路径以 `cli; hlt; jmp` 等待，未启用中断、定时器或任务。`hlt` 后可能因 NMI/debug
恢复，循环会再次暂停。尚无自有异常处理，意外异常可能导致 triple fault；Phase 2 解决此限制。

失败路径设置 `stage=0xff`，输出 `PANIC` 原因并停机。无 framebuffer 时串口仍可诊断；
没有串口且没有屏幕时无法显示诊断。串口等待有上限，缺失设备不会无限阻塞屏幕输出。

## C 与 ELF

链接基址 `0xffffffff80000000`。请求、代码、只读数据、可写数据分别使用独立 PT_LOAD，
无动态链接、无 libc、无可写可执行 ELF 段、无可执行 GNU 栈要求。实际页表来自 Limine，
ELF 段标志不能代替未来的页表权限测试。

编译禁用 red zone、栈保护器、PIE、浮点/SIMD 寄存器生成与宿主内建内存函数，
以 `-ffreestanding -mcmodel=kernel` 生成 x86-64 代码。当前不依赖浮点状态初始化。
`memcpy/memmove/memset/memcmp` 自行实现，不依赖宿主 C 库。

测试会实际断在 C 入口检查栈、DF/IF、BSS，再在 idle 前检查诊断数据；不是仅检查日志文字。
