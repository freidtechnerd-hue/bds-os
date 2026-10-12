ENTRY(_start)
SECTIONS {
 . = 1M;
 .multiboot : { KEEP(*(.multiboot)) }
 .text ALIGN(4096) : { *(.text*) }
 .rodata ALIGN(4096) : { *(.rodata*) }
 .data ALIGN(4096) : { *(.data*) }
 .bss ALIGN(4096) : { *(COMMON) *(.bss*) }
 kernel_end = .;
 ASSERT(kernel_end < 0x800000, "Kernel overlaps the native app slot")
 /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
