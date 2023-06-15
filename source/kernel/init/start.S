    .text
    .extern kernel_init
    .global _start
    # void start(boot_info_t *boot_info)
_start:
    mov 0x4(%esp), %eax

    # kernel_init(boot_info)
    push %eax
    call kernel_init
    jmp .
