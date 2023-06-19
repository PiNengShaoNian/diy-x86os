#include "core/task.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "cpu/cpu.h"

static int tss_init(task_t *task, uint32_t entry, uint32_t esp)
{
    kernel_memset(&task->tss, 0, sizeof(tss_t));
    task->tss.eip = entry;
    task->tss.esp = task->tss.esp0 = esp;
    task->tss.ss = task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = KERNEL_SELECTOR_DS;
    task->tss.cs = KERNEL_SELECTOR_CS;
    task->tss.eflags = EFLAGS_IF | EFLAGS_DEFAULT;
    return 0;
}

int task_init(task_t *task, uint32_t entry, uint32_t esp)
{
    ASSERT(task != (task_t *)0);

    tss_init(task, entry, esp);
    return 0;
}