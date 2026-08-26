MEMORY
{
    FLASH (rx) : ORIGIN = 0x00002000, LENGTH = 248K 
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 32K
}

ENTRY(Reset_Handler)

/* Define minimum stack size (2 KB) */
_Min_Stack_Size = 0x2000; 

SECTIONS {
    .text : {
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
        *(.eh_frame*)

        . = ALIGN(4);
    } > FLASH

    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .init_array : {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        __init_array_end = .;
        . = ALIGN(4);
    } > FLASH

    _etext = .;

    .data : {
        _sdata = .;
        __sdata = .;
        *(.data*)
        _edata = .;
        __edata = .;
    } > RAM AT> FLASH

    /* Source address in FLASH for the .data copy at startup (must be the
     * .data LMA, NOT the end of .text - there are other FLASH sections
     * (.ARM.extab/.ARM/.init_array) between .text and the .data LMA). */
    _sidata = LOADADDR(.data);

    .bss : {
        _sbss = .;
        __sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
        __ebss = .;
    } > RAM

    /* Reserve guaranteed stack space at the top of RAM */
    ._user_stack : {
        . = ALIGN(8);
        . += _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* Set top of stack symbol to the very end of RAM (0x20008000) */
    PROVIDE(_estack = ORIGIN(RAM) + LENGTH(RAM));
}