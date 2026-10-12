ENTRY(app_main)
SECTIONS {
 . = 0x800000;
 .text : { *(.text.app_main) *(.text*) }
 .rodata : { *(.rodata*) }
 .data : { *(.data*) }
 image_end = .;
 .bss : { *(COMMON) *(.bss*) }
 memory_end = .;
 /DISCARD/ : { *(.eh_frame*) *(.comment) *(.note*) }
}
