# Built-in application development

Hello is independently compiled from apps/hello.c using apps/linker.ld and apps/abi.h:

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
Inspect the Makefile APPS rules: freestanding static ELF is converted by objcopy to a fixed flat image and embedded by apps.S. The kernel does not parse ELF headers. Edit an existing Hello literal and rebuild to observe a change; do not add writable globals/BSS or libc dependencies. finish must be used because _start has no ordinary return caller. Counter polls real uptime until50 ticks pass, prints its initial tick value, yields and exits. Adding a new image also requires APPS, user.c whitelist/symbols and apps command updates; this is not dynamic installation.
[System calls](syscalls.md) · [Limits](limitations.md)

[Index](index.md)
