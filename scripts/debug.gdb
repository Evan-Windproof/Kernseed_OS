set pagination off
set disassembly-flavor intel
file build/kernseed.elf
target remote 127.0.0.1:1234
hbreak *kernel_main
continue
