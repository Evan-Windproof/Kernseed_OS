ENTRY(_start)
SECTIONS { . = 0xffffc00001000000; .text : { *(.text*) } .rodata : { *(.rodata*) } .data : { *(.data*) } .bss : { *(.bss*) } /DISCARD/ : { *(.eh_frame*) *(.note*) *(.comment*) } }
