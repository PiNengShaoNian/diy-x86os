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

void kernel_init(boot_info_t *boot_info)
{
    ASSERT(boot_info->ram_region_count != 0);

    cpu_init();

    log_init();
    irq_init();
    time_init();
}

static task_t first_task;
static uint32_t init_task_stack[1024];
static task_t init_task;

void init_task_entry(void)
{
    int count = 0;
    for (;;)
    {
        log_printf("int task: %d", count++);
        task_switch_from_to(&init_task, &first_task);
    }
}

void list_test(void)
{
    list_t list;
    list_init(&list);
}

void init_main()
{
    list_test();
    log_printf("Kernel is running...");
    log_printf("Version: %s %s", OS_VERSION, "diy x86-os");
    log_printf("%d %d %x %c 0x%x", -123, 123456, 0x12345, 'a', 15);

    task_init(&init_task, (uint32_t)init_task_entry, (uint32_t)&init_task_stack[1024]);
    task_init(&first_task, (uint32_t)0, 0);
    write_tr(first_task.tss_sel);

    int count = 0;
    for (;;)
    {
        log_printf("main task: %d", count++);
        task_switch_from_to(&first_task, &init_task);
    }
}