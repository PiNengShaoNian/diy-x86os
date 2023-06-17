#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"

void kernel_init(boot_info_t *boot_info)
{
    cpu_init();
    irq_init();
}

void init_main()
{
    int a = 3 / 0;
}