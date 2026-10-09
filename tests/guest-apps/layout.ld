ENTRY(_start)
SECTIONS {
 . = 0x2000;
 .text : { *(.text.start) *(.text*) *(.rodata*) }
 . = 0x20000;
 .data : { *(.data.imports) *(.data*) *(.bss*) *(COMMON) }
 /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment*) }
}
