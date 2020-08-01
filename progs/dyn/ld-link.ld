ENTRY(_start);

SECTIONS {
    . = 0x200000;

    _ld_start = .;
    .text : ALIGN(4K) {
        *(.init .fini)
        *(.text .text.*)
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4K) {
        *(.ctors .dtors)
        *(.data .data.*)
    }

    .bss : ALIGN(4K) {
        *(.bss)
        *(COMMON)
    }
    _ld_end = .;
}
