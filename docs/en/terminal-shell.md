# Terminal and Shell

IRQ1 decodes translated i8042 Set1 into a255-character usable ring. IRQ code neither prints nor parses. shell_task sleeps when empty, edits a bounded line and invokes parser → registry → dispatcher → handler. terminal_* hides serial/framebuffer details. help traverses linker-registered descriptors. Input is ASCII, case sensitive, rendered with a limited uppercase font; UART is output-only. Maximum127 input bytes and8 tokens including command; quotes, escapes and empty arguments are supported. Overflow discards the whole line; press Enter to recover. Backspace and left Ctrl-U edit; no pipes/history/POSIX expansion.

[Source / 源码](../../shell/dispatch.c) · [Index / 目录](index.md)

## Practice and verification

```sh
make test-shell
```

Observe actual guest state: keyboard_state / shell_state / framebuffer pixels / reboot. Tests require QEMU/GDB on PATH (or QEMU/GDB overrides). Fault fixtures use separate output directories; normal boot does not inject them. Read the linked source alongside the GDB checks before changing it.
