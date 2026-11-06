# 编写 Kernseed 小程序

这是Kernseed原生最小ABI，不能运行Linux ELF、libc或POSIX程序。
用户程序独立编译，不包含kernel头文件，不由内核直接调用C函数。

完整Hello示例（apps/hello.c）：

```c
/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include "abi.h"
void _start(void) {
    write_text("Hello from Kernseed userspace!\n");
    finish();
}
```

apps/abi.h使用RAX/RDI/RSI和真实int 0x82。apps/linker.ld定址到USER_CODE；应用单独freestanding静态编译，
不链接内核或宿主CRT，objcopy转换平坦映像。Makefile APPS列表和user.c内置映像白名单配套管理，
不是任意ELF加载。新增应用必须保持总映像≤4 KiB、无可写全局/BSS，使用4 KiB栈，主动finish。

```sh
make iso
make run
# QEMU键盘：
apps
run hello
run counter
run probe
```

Counter计算并查询真实tick、执行yield后退出。probe检查无效编号/空指针/内核指针/跨界指针。
privileged/kernelread/kernelwrite/badflags/badstack为独立危险程序，只由显式run或测试启动，正常启动不自动触发。
它们验证权限故障、标志过滤和恶意栈处理，故障后Shell应继续可用。

没有动态链接、文件系统、argv/envp、用户堆或多进程并发。run同步等待用户任务结束，无外部kill；
无限循环程序可能长期占据唯一用户名额，但PIT仍能调度内核任务。
