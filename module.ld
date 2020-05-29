ENTRY(_mod_entry);

SECTIONS {
    /*
     * Modules are linked with 0x0 base virtual address
     * and are position independent
     */
    . = 0x100000000;

    .text : ALIGN(4K) {
        *(.text*)
    }

    .rodata : {
        *(.rodata)
        *(.eh_frame)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : {
        *(COMMON)
        *(.bss*)
    }
}
