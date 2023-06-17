#include "os_cfg.h"

    .text
    .extern kernel_init
    .global _start
    .extern init_main
    # void start(boot_info_t *boot_info)
_start:
    mov 0x4(%esp), %eax

    # kernel_init(boot_info)
    push %eax
    call kernel_init

    jmp $KERNEL_SELECTOR_CS, $gdt_reload

gdt_reload:
    mov $KERNEL_SELECTOR_DS, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

    mov $(stack + KERNEL_STACK_SIZE), %esp
    jmp init_main

    .bss
.comm stack, KERNEL_STACK_SIZE

    .text
    .extern do_handler_unknown
    .global exception_handler_unknown
exception_handler_unknown:
    pusha
    push %ds
    push %es
    push %fs
    push %gs

    push %esp
    call do_handler_unknown
    add $4, %esp
    
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    
    iret
