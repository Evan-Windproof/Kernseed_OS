# 内置应用开发

Hello使用apps/linker.ld与apps/abi.h独立编译，源码为apps/hello.c：

```c
/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include "abi.h"
void _start(void){write_text("Hello from Kernseed userspace!\n");finish();}
```
```sh
make iso
make build/apps/hello.elf
make run
# Guest keyboard: apps, run hello, run counter
```
查看Makefile的APPS规则：freestanding静态ELF经objcopy转定址平坦映像，由apps.S嵌入；内核不解析ELF头。修改Hello字面量后重建可观察变化，不添加可写全局/BSS或libc依赖。_start没有普通调用者，必须finish退出。Counter轮询真实uptime至50tick，输出起始tick、yield并退出。新增映像还需修改APPS、user.c白名单/符号及apps命令；不是动态安装。
[System calls](syscalls.md) · [Limits](limitations.md)

[Index](index.md)
