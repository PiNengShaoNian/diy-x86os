#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "dev/console.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "core/task.h"
#include "tools/list.h"
#include "core/memory.h"
#include "dev/kbd.h"
#include "fs/fs.h"

void kernel_init(boot_info_t *boot_info)
{
    ASSERT(boot_info->ram_region_count != 0);

    cpu_init();
    irq_init();
    log_init();

    memory_init(boot_info);
    fs_init();
    time_init();

    task_manager_init();
}

void move_to_first_task(void)
{
    task_t *curr = task_current();
    ASSERT(curr != 0);
    tss_t *tss = &(curr->tss);
    __asm__ __volatile__("push %[ss]\n\t"
                         "push %[esp]\n\t"
                         "push %[eflags]\n\t"
                         "push %[cs]\n\t"
                         "push %[eip]\n\t"
                         "iret" ::[ss] "r"(tss->ss),
                         [esp] "r"(tss->esp),
                         [eflags] "r"(tss->eflags),
                         [cs] "r"(tss->cs),
                         [eip] "r"(tss->eip));
}

void init_main()
{
    log_printf("Kernel is running...");
    log_printf("Version: %s %s", OS_VERSION, "diy x86-os");

    task_first_init();
    move_to_first_task();
}