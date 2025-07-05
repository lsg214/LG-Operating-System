/* Read-only data section */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
    }
    
    /* Initialized data section */
    .data ALIGN(4K) :
    {
        *(.data)
    }
    
    /* Uninitialized data section */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }
    
    /* Stack section (grows downward) */
    .stack ALIGN(4K) :
    {
        . += 16K;  /* 16KB stack */
        stack_top = .;
    }
    
    /* Provide symbols for memory layout */
    kernel_start = 0x100000;
    kernel_end = .;
    kernel_size = kernel_end - kernel_start;
}