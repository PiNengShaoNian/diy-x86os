#include "comm/boot_info.h"
#include "cpu/cpu.h"

void kernel_init(boot_info_t *boot_info)
{
    cpu_init();
}

void init_main()
{
}