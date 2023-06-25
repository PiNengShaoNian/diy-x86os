#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "core/task.h"
#include "tools/list.h"
#include "core/memory.h"

void kernel_init(boot_info_t *boot_info)
{
    ASSERT(boot_info->ram_region_count != 0);

    cpu_init();
    log_init();

    memory_init(boot_info);

    irq_init();
    time_init();

    task_manager_init();
}

void move_to_first_task(void)
{
    task_t *curr = task_current();
    ASSERT(curr != 0);
    tss_t *tss = &(curr->tss);
    __asm__ __volatile__("jmp *%[ip]" ::[ip] "r"(tss->eip));
}

void init_main()
{
    log_printf("Kernel is running...");
    log_printf("Version: %s %s", OS_VERSION, "diy x86-os");
    log_printf("%d %d %x %c 0x%x", -123, 123456, 0x12345, 'a', 15);

    task_first_init();
    move_to_first_task();
}