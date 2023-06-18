#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "os_cfg.h"

void kernel_init(boot_info_t *boot_info)
{
    ASSERT(boot_info->ram_region_count != 0);

    cpu_init();

    log_init();
    irq_init();
    time_init();
}

void init_main()
{
    log_printf("Kernel is running...");
    log_printf("Version: %s %s", OS_VERSION, "diy x86-os");
    log_printf("%d %d %x %c 0x%x", -123, 123456, 0x12345, 'a', 15);

    int a = 3 / 0;
    // irq_enable_global();
    for (;;)
        ;
}